#include "irc.h"
#include "types.h"

#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define SHAREDMEM_SIZE 0x1000
#define SHAREDMEM_ALIGNMENT 0x1000

static void* sharedMem;
static u8 transferBuffer[0x208];
static Handle recvEvent;

// https://chromium.googlesource.com/chromiumos/platform/vboot_reference/+/stabilize2/firmware/lib/crc8.c
static uint8_t crc8(const void* vptr, int len) {
    const uint8_t *data = vptr;
    unsigned crc = 0;
    int i, j;
    for (j = len; j; j--, data++) {
        crc ^= (*data << 8);
        for(i = 8; i; i--) {
            if (crc & 0x8000)
                crc ^= (0x1070 << 3);
            crc <<= 1;
        }
    }
    return (uint8_t)(crc >> 8);
}

int IRCInit(u8 bitrate)
{
	sharedMem = memalign(SHAREDMEM_ALIGNMENT, SHAREDMEM_SIZE);
    if (!sharedMem) {
        return -1;
    }

	if (R_FAILED(iruInit(sharedMem, SHAREDMEM_SIZE))) {
		printf("IRCInit: IRU_Initialize failed\n");
        return -1;
	}

	if (R_FAILED(IRU_SetBitRate(bitrate))) {
        return -1;
    }

	if (R_FAILED(IRU_GetRecvFinishedEvent(&recvEvent))) {
        return -1;
    }

    return 0;
}

int IRCShutdown(void)
{
    iruExit();
    free(sharedMem);

    return 0;
}

int IRCSend(void* data, u32 size)
{
    void *packetData;
    uint32_t packetSize;
    if (size < 0x3e) {
        CCRCDCIrdaSmallPacketHeader* header = (CCRCDCIrdaSmallPacketHeader*) transferBuffer;
        header->sessionId = 0;
        header->receiveSize = 0;
        header->magic = 0xa5;
        header->unk = 0;
        header->isLarge = 0;
        header->dataSize = size + 2; // +2 for requestSize

        packetData = header + 1;
        packetSize = size + sizeof(CCRCDCIrdaSmallPacketHeader) + 1; // +1 for crc8
    } else {
        CCRCDCIrdaLargePacketHeader* header = (CCRCDCIrdaLargePacketHeader*) transferBuffer;
        header->sessionId = 0;
        header->receiveSize = 0;
        header->magic = 0xa5;

        uint16_t totalSize = size + 2; // +2 for requestSize
        header->flags = __builtin_bswap16((totalSize & 0x3fff) | 0x4000);

        packetData = header + 1;
        packetSize = size + sizeof(CCRCDCIrdaLargePacketHeader) + 1; // +1 for crc8
    }

    // Copy the data into the packet
    memcpy(packetData, data, size);

    // Calculate crc
    transferBuffer[packetSize - 1] = crc8(transferBuffer, packetSize - 1);

    // Send the packet
    return R_FAILED(iruSendData(transferBuffer, packetSize, true)) ? -1 : 0;
}

int IRCReceive(void* data, u32 bufferSize, u32* receivedSize, u32 timeout)
{
    if (R_FAILED(IRU_StartRecvTransfer(bufferSize + sizeof(CCRCDCIrdaLargePacketHeader) + 1, 0))) {
        return -1;
    }

    svcWaitSynchronization(recvEvent, timeout * 1000000llu);
    svcClearEvent(recvEvent);

    u32 transferCount;
    if (R_FAILED(IRU_WaitRecvTransfer(&transferCount))) {
        return -1;
    }

    if (!transferCount) {
        return 0;
    }

    u8* buffer = (u8*) sharedMem;

    // verify magic
    if (buffer[0] != 0xa5) {
        printf("IRCReceive: wrong magic (%x)\n", buffer[0]);
        return -1;
    }

    // verify crc
    if (buffer[transferCount - 1] != crc8(buffer, transferCount - 1)) {
        printf("IRCReceive: crc mismatch\n");
        return -1;
    }

    CCRCDCIrdaSmallPacketHeader* smallHeader = (CCRCDCIrdaSmallPacketHeader*) buffer;
    CCRCDCIrdaLargePacketHeader* largeHeader = (CCRCDCIrdaLargePacketHeader*) buffer;

    void *packetData;
    uint16_t packetSize;
    if (smallHeader->isLarge) {
        packetData = largeHeader + 1;
        packetSize = __builtin_bswap16(largeHeader->flags) & 0x3fff;
    } else {
        packetData = smallHeader + 1;
        packetSize = smallHeader->dataSize;
    }

    // don't count the request size
    packetSize -= 2;

    if (packetSize > 0x1fe) {
        return -1;
    }

    *receivedSize = packetSize;
    memcpy(data, packetData, packetSize);

    return 0;
}
