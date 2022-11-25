#include <whb/proc.h>
#include <whb/gfx.h>
#include <stdio.h>

#include <irc/irc.h>
#include <irc/cdc.h>

#include "console.h"
#include "font.h"
#include "swkbd.h"

enum {
    MESSAGE_ANNOUNCEMENT = 0xa0,
    MESSAGE_TEXT = 0xa1,
    MESSAGE_ACK = 0xa2,
};

static int pendingTextSendSize = 0;
static uint8_t textSendBuffer[CCR_CDC_IRDA_DATA_TRANSFER_SIZE];

static int sendAck(uint32_t size)
{
    uint8_t ack[] = { MESSAGE_ACK };
    return IRCSend(VPAD_CHAN_0, ack, sizeof(ack), size);
}

static int sendText(const char* txt)
{
    size_t len = strlen(txt);
    if (len >= sizeof(textSendBuffer) - 1) {
        len = sizeof(textSendBuffer) - 1;
    }

    // Prepare the text
    textSendBuffer[0] = MESSAGE_TEXT;
    memcpy(textSendBuffer + 1, txt, len);
    pendingTextSendSize = len + 1;

    // Send the announcement
    uint8_t announcement[] = { MESSAGE_ANNOUNCEMENT, (len >> 8) & 0xff, len & 0xff };
    if (IRCSend(VPAD_CHAN_0, announcement, sizeof(announcement), 1) != 0) {
        printf(">Failed to send announcement\n");
        return -1;
    }

    return 0;
}

static void connectCallback()
{
    printf(">IR connection initialized\n");
}

static void receiveCallback(void* data, uint16_t size, IRCResult result)
{
    if (!data || !size || result != 0) {
        printf(">Failed to receive message: %d\n", result);
        return;
    }

    uint8_t* msg = (uint8_t*) data;
    if (msg[0] == MESSAGE_ANNOUNCEMENT) {
        uint16_t textSize = ((uint16_t) msg[1] << 8) | msg[2];

        if (sendAck(textSize + 1) != IRC_RESULT_SUCCESS) {
            printf(">Failed to send acknowledgement\n");
        }
    } else if (msg[0] == MESSAGE_TEXT) {
        printf("3DS: %*.*s\n", size - 1, size - 1, msg + 1);

        if (sendAck(3) != IRC_RESULT_SUCCESS) {
            printf(">Failed to send acknowledgement\n");
        }
    } else if (msg[0] == MESSAGE_ACK) {
        if (pendingTextSendSize > 0) {
            if (IRCSend(VPAD_CHAN_0, textSendBuffer, pendingTextSendSize, 3) != IRC_RESULT_SUCCESS) {
                printf(">Failed to send text\n");
            }

            pendingTextSendSize = 0;
        }
    } else {
        printf(">Unknown message received\n");
    }
}

int main(int argc, char const *argv[])
{
    WHBProcInit();

    // Initialize graphics
    WHBGfxInit();
    Console_Init();

    // Initialize keyboard
    swkbdInit();

    // Draw header
    printf("\u250f\u2501\u2501\u2501\u2501\u2501\n");
    printf("\u2503 InfraRedChat (Wii U)\n");
    printf("\u2503 Press \ue000 to enter text\n");
    printf("\u2517\u2501\u2501\u2501\u2501\u2501\u2501"
        "\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u25b7\n");
    Console_Draw();

    // Make sure there is no active connection from another app which didn't clean up properly
    uint8_t result;
    __CCRCDCIRCDisconnect(0, &result);

    // Initialize IRC
    if (!IRCInit(VPAD_CHAN_0, 0xdc)) {
        printf("IRCInit failed\n");
        Console_Draw();
    }

    IRCResult res = IRCConnect(VPAD_CHAN_0, 0, CCR_IRDA_CONNECTION_ANY, CCR_IRDA_BITRATE_115200, 3, connectCallback);
    if (res != IRC_RESULT_SUCCESS) {
        printf("IRCConnect failed\n");
        Console_Draw();
    }

    IRCSetReceiveCallback(VPAD_CHAN_0, receiveCallback);

    while (WHBProcIsRunning()) {
        IRCProc(VPAD_CHAN_0);

        VPADStatus status = { 0 };
        VPADRead(VPAD_CHAN_0, &status, 1, NULL);
        VPADGetTPCalibratedPoint(VPAD_CHAN_0, &status.tpNormal, &status.tpNormal);

        if (swkbdIsOpened()) {
            const char* text = swkbdProc(&status);
            if (text) {
                printf("WiiU: %s\n", text);
                sendText(text);
            }
        }

        if (status.trigger & VPAD_BUTTON_A && !swkbdIsOpened()) {
            swkbdShow();
        }

        Console_Draw();

        WHBGfxBeginRender();
        WHBGfxBeginRenderTV();
        {
            WHBGfxClearColor(GX2_CLEAR_COLOR);

            Font_Draw();
            swkbdDrawTV();
        }
        WHBGfxFinishRenderTV();

        WHBGfxBeginRenderDRC();
        {
            WHBGfxClearColor(GX2_CLEAR_COLOR);

            Font_Draw();
            swkbdDrawDRC();
        }
        WHBGfxFinishRenderDRC();
        WHBGfxFinishRender();
    }

    IRCDisconnect(VPAD_CHAN_0);

    swkbdExit();
    Console_Shutdown();

    WHBGfxShutdown();
    WHBProcShutdown();
    return 0;
}

