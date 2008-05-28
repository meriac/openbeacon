#ifndef __OPENBEACON_H__
#define __OPENBEACON_H__

#define TEA_ENCRYPTION_BLOCK_COUNT 4

#define RFB_RFOPTIONS		0x0F

#define RFBPROTO_BEACONTRACKER	23

#define RFBFLAGS_ACK		0x01
#define RFBFLAGS_SENSOR		0x02

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    u_int8_t size,proto;
} TBeaconHeader;

typedef struct
{
   u_int8_t size,proto;
//    TBeaconHeader hdr;  // 2  On ARM, composed struct is aligned to 4!!!
    u_int8_t flags,strength; // 2
    u_int32_t seq; // 4
    u_int32_t oid;  // 4
    u_int16_t reserved;  // 2
    u_int16_t crc; // 2
} TBeaconTracker;

typedef union
{
    TBeaconTracker pkt;
    u_int32_t data[TEA_ENCRYPTION_BLOCK_COUNT];
    u_int8_t datab[TEA_ENCRYPTION_BLOCK_COUNT*sizeof(u_int32_t)];
} TBeaconEnvelope;
#pragma pack(pop)

#endif/*__OPENBEACON_H__*/
