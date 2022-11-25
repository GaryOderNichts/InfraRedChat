#pragma once

#include <3ds.h>

int IRCInit(u8 bitrate);

int IRCShutdown(void);

int IRCSend(void* data, u32 size);

int IRCReceive(void* data, u32 bufferSize, u32* receivedSize, u32 timeout);
