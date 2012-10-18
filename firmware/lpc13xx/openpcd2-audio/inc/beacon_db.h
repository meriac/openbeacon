#ifndef __OB_DATABASE_H__
#define __OB_DATABASE_H__

#define DB_DIR_ENTRY_TYPE_ROOT 1
#define DB_DIR_ENTRY_TYPE_VIDEO_GREY 2
#define DB_DIR_ENTRY_TYPE_AUDIO_8BIT 3

typedef struct {
	uint8_t type;
	uint8_t flags;
	uint16_t id, next_id;
	uint32_t pos;
	uint32_t length;
	uint16_t crc16;
} PACKED TBeaconDbDirEntry;

#endif/*__OB_DATABASE_H__*/