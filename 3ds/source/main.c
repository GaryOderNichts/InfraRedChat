#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "irc.h"

// 115200 bps
#define BITRATE 3

enum {
    MESSAGE_ANNOUNCEMENT = 0xa0,
    MESSAGE_TEXT = 0xa1,
    MESSAGE_ACK = 0xa2,
};

static u8 sendBuffer[0x208];
static u8 receiveBuffer[0x208];

static SwkbdState swkbd;
static char textBuffer[0x200];

static int waitForAck(u32 timeout)
{
    uint8_t ack; uint32_t ack_size;
    if (IRCReceive(&ack, sizeof(ack), &ack_size, timeout) != 0) {
        return -1;
    }

    if (ack_size != sizeof(ack)) {
        return -1;
    }

    if (ack != MESSAGE_ACK) {
        return -1;
    }

    return 0;
}

static int sendText(const char* txt)
{
    size_t len = strlen(txt);
    if (len >= sizeof(sendBuffer) - 1) {
        len = sizeof(sendBuffer) - 1;
    }

    // Send the announcement
    uint8_t announcement[] = { MESSAGE_ANNOUNCEMENT, (len >> 8) & 0xff, len & 0xff };
    if (IRCSend(announcement, sizeof(announcement)) != 0) {
        printf(">Failed to send announcement\n");
        return -1;
    }

    // Wait for the acknowledgement
    if (waitForAck(500) != 0) {
        printf(">Failed to receive acknowledgement\n");
        return -1;
    }

    // Send the text
    sendBuffer[0] = MESSAGE_TEXT;
    memcpy(sendBuffer + 1, txt, len);
    if (IRCSend(sendBuffer, len + 1) != 0) {
        printf(">Failed to send text\n");
        return -1;
    }

    // Don't really care about this one but still try to receive it
    waitForAck(500);
    return 0;
}

static int receiveText()
{
    u8 msg[3];
    uint32_t msg_size;
    if (IRCReceive(msg, sizeof(msg), &msg_size, 100) != 0) {
        printf(">Failed to receive message\n");
    }

    if (!msg_size) {
        return 0;
    }

    if (msg[0] == MESSAGE_ANNOUNCEMENT && msg_size == 3) {
        uint16_t textSize = ((uint16_t) msg[1] << 8) | msg[2];
        if (textSize > sizeof(receiveBuffer) - 1) {
            return -1;
        }

        uint8_t ack = MESSAGE_ACK;
        if (IRCSend(&ack, sizeof(ack)) != 0) {
            printf(">Failed to send acknowledgement\n");
            return -1;
        }

        if (IRCReceive(receiveBuffer, textSize + 1, &msg_size, 1500) != 0) {
            printf(">Failed to receive text\n");
            return -1;
        }

        if (msg_size) {
            printf("WiiU: %*.*s\n", textSize, textSize, receiveBuffer + 1);
            return 0;
        }
    }

    return -1;
}

int main(int argc, char const *argv[])
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("InfraRedChat (3DS)\nPress (A) to enter text.\n\n");

    if (IRCInit(BITRATE) != 0) {
        printf(">Failed to init IR connection\n");
    }

    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

        if (kDown & KEY_A) {
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, sizeof(textBuffer));
            if (swkbdInputText(&swkbd, textBuffer, sizeof(textBuffer)) == SWKBD_BUTTON_CONFIRM) {
                printf("3DS: %s\n", textBuffer);
                sendText(textBuffer);
            }
        }

        receiveText();
    }

    IRCShutdown();

    iruExit();
    gfxExit();
    return 0;
}

