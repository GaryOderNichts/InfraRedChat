#pragma once

#include <3ds.h>

typedef struct PACKED CCRCDCIrdaSmallPacketHeader
{
   //! Magic value (needs to be set to 0xa5)
   uint8_t magic;
   //! ID of the session (gets filled out by the DRC before transmitting the packet)
   uint8_t sessionId;
   /* bitfield swapped for little endian */
   //! Size of the actual data
   uint8_t dataSize : 6;
   //! 0 for small packets
   uint8_t isLarge : 1;
   uint8_t unk : 1;
   //! Amount of data which should be received from the other device
   uint16_t receiveSize;
} CCRCDCIrdaSmallPacketHeader;

typedef struct PACKED CCRCDCIrdaLargePacketHeader
{
   //! Magic value (needs to be set to 0xa5)
   uint8_t magic;
   //! ID of the session (gets filled out by the DRC before transmitting the packet)
   uint8_t sessionId;
   /* can't really use a nice bitfield here due to endianness */
   //! Size of the actual data + isLarge + unk
   uint16_t flags;
   //! Amount of data which should be received from the other device
   uint16_t receiveSize;
} CCRCDCIrdaLargePacketHeader;
