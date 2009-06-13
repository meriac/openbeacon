#ifndef EINK_H_
#define EINK_H_

#define EINK_CMD_INIT_CMD_SET 0x00
#define EINK_CMD_INIT_PLL_STBY 0x01
#define EINK_CMD_RUN_SYS 0x02
#define EINK_CMD_STBY 0x04
#define EINK_CMD_SLP 0x05
#define EINK_CMD_INIT_SYS_RUN 0x06
#define EINK_CMD_INIT_CMD_STBY 0x07
#define EINK_CMD_INIT_SDRAM 0x08
#define EINK_CMD_INIT_DSPE_CFG 0x09
#define EINK_CMD_INIT_DSPE_TMG 0x0A
#define EINK_CMD_INIT_ROTMODE 0x0B

#define EINK_CMD_RD_REG 0x10
#define EINK_CMD_WR_REG 0x11
#define EINK_CMD_RD_SFM 0x12
#define EINK_CMD_WR_SFM 0x13
#define EINK_CMD_END_SFM 0x14

#define EINK_CMD_BST_RD_SDR 0x1C
#define EINK_CMD_BST_WR_SDR 0x1D
#define EINK_CMD_BST_END_SDR 0x1E

#define EINK_CMD_LD_IMG 0x20
#define EINK_CMD_LD_IMG_AREA 0x22
#define EINK_CMD_LD_IMG_END 0x23
#define EINK_CMD_LD_IMG_WAIT 0x24
#define EINK_CMD_LD_IMG_SETADR 0x25
#define EINK_CMD_LD_IMG_DSPEADR 0x26

#define EINK_CMD_WAIT_DSPE_TRG 0x28
#define EINK_CMD_WAIT_DSPE_FREND 0x29
#define EINK_CMD_WAIT_DSPE_LUTFREE 0x2A
#define EINK_CMD_WAIT_DSPE_MLUTFREE 0x2B

#define EINK_CMD_RD_WFM_INFO 0x30
#define EINK_CMD_UPD_INIT 0x32
#define EINK_CMD_UPD_FULL 0x33
#define EINK_CMD_UPD_FULL_AREA 0x34
#define EINK_CMD_UPD_PART 0x35
#define EINK_CMD_UPD_PART_AREA 0x36
#define EINK_CMD_UPD_GDRV_CLR 0x37
#define EINK_CMD_UPD_SET_IMGADR 0x38

#define EINK_WAVEFORM_SELECT(a) (a<<8)

/* Initialisation to white (and only to white), ~4000 ms */
#define EINK_WAVEFORM_INIT EINK_WAVEFORM_SELECT(0)

/* Direct update, black/white only, 260ms */
#define EINK_WAVEFORM_DU EINK_WAVEFORM_SELECT(1)

/* Gray update, 8 gray levels, 780ms, slight ghosting */
#define EINK_WAVEFORM_GU EINK_WAVEFORM_SELECT(2)

/* Gray clear, 8 gray levels, 780ms, high flashing */
#define EINK_WAVEFORM_GC EINK_WAVEFORM_SELECT(3)

#define EINK_LUT_SELECT(a) (a<<4)

#define EINK_ROTATION_MODE(a) (a<<8)
#define EINK_ROTATION_MODE_0 EINK_ROTATION_MODE(0)
#define EINK_ROTATION_MODE_90 EINK_ROTATION_MODE(1)
#define EINK_ROTATION_MODE_180 EINK_ROTATION_MODE(2)
#define EINK_ROTATION_MODE_270 EINK_ROTATION_MODE(3)

#define EINK_PACKED_MODE(a) (a<<4)
#define EINK_PACKED_MODE_2BIT EINK_PACKED_MODE(0)
#define EINK_PACKED_MODE_3BIT EINK_PACKED_MODE(1)
#define EINK_PACKED_MODE_4BIT EINK_PACKED_MODE(2)
#define EINK_PACKED_MODE_1BYTE EINK_PACKED_MODE(3)

enum eink_error {
	EINK_ERROR_NO_ERROR=0,
	EINK_ERROR_NOT_DETECTED,
	EINK_ERROR_NOT_SUPPORTED,
	EINK_ERROR_COMMUNICATIONS_FAILURE,
	EINK_ERROR_CHECKSUM_ERROR,
};

#define EINK_FLASH_CONTROL_VALUE  0x0099

enum eink_display_type {
	EINK_DISPLAY_60,
	EINK_DISPLAY_80,
	EINK_DISPLAY_97,
};
struct eink_display_configuration {
	int hsize, vsize,
		fslen, fblen, felen,
		lslen, lblen, lelen,
		pixclkdiv,
		sdrv_cfg,
		gdrv_cfg,
		lutidxfmt;
};
extern const struct eink_display_configuration EINK_DISPLAY_CONFIGURATIONS[];
extern const struct eink_display_configuration *EINK_CURRENT_DISPLAY_CONFIGURATION;

#define _EINK_ROUND_UP_32(i) ( ((i+31)/32)*32  )
#define EINK_IMAGE_SIZE (_EINK_ROUND_UP_32(EINK_CURRENT_DISPLAY_CONFIGURATION->hsize) *  \
	EINK_CURRENT_DISPLAY_CONFIGURATION->vsize) 
#define EINK_UPDATE_BUFFER_START 0x0
#define EINK_IMAGE_BUFFER_START ( EINK_IMAGE_SIZE * 2)

extern void eink_controller_reset(void);
extern int eink_controller_init(void);
extern void eink_interface_init(void);

/* The following come from eink_mgmt.c and should be somewhat hardware-independent. */

extern int eink_mgmt_init(void *memory, unsigned int size);
extern const unsigned int eink_mgmt_image_buffer_size;
extern const unsigned int eink_mgmt_job_size;
extern const unsigned int eink_mgmt_size;

typedef struct eink_image_buffer *eink_image_buffer_t;
typedef struct eink_job *eink_job_t;

enum eink_pack_mode {
	PACK_MODE_2BIT,
	PACK_MODE_3BIT,
	PACK_MODE_4BIT,
	PACK_MODE_1BYTE,
};

enum eink_rotation_mode {
	ROTATION_MODE_0,
	ROTATION_MODE_90,
	ROTATION_MODE_180,
	ROTATION_MODE_270,
};

enum eink_waveform_mode {
	WAVEFORM_MODE_INIT,
	WAVEFORM_MODE_DU,
	WAVEFORM_MODE_GU,
	WAVEFORM_MODE_GC,
};

enum eink_update_mode {
	UPDATE_MODE_FULL,
	UPDATE_MODE_PART,
	UPDATE_MODE_PART_SPECIAL,
	UPDATE_MODE_INIT,
};

extern int eink_image_buffer_acquire(eink_image_buffer_t *buf);
extern int eink_image_buffer_load(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode, 
		const unsigned char *data, unsigned int length);
extern int eink_image_buffer_load_area(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h, 
		const unsigned char *data, unsigned int length);
extern int eink_image_buffer_load_begin(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode);
extern int eink_image_buffer_load_begin_area(eink_image_buffer_t buf, 
		enum eink_pack_mode pack_mode, enum eink_rotation_mode rotation_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h);
extern int eink_image_buffer_load_stream(eink_image_buffer_t buf, 
		const unsigned char *data, unsigned int length);
extern int eink_image_buffer_load_end(eink_image_buffer_t buf);
extern int eink_image_buffer_release(eink_image_buffer_t *buf);

extern int eink_job_begin(eink_job_t *job, int cookie);
extern int eink_job_add(eink_job_t job, eink_image_buffer_t buf,
		enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode);
extern int eink_job_add_area(eink_job_t job, eink_image_buffer_t buf,
		enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h);
extern int eink_job_add_area_with_offset(eink_job_t job, eink_image_buffer_t buf,
		enum eink_waveform_mode waveform_mode, enum eink_update_mode update_mode,
		unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int xoffset, unsigned int yoffset);
extern int eink_job_commit(eink_job_t job);

extern int eink_job_count_pending(void);

#endif /*EINK_H_*/
