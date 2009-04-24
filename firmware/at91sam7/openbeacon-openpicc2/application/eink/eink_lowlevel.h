#ifndef EINK_LOWLEVEL_H_
#define EINK_LOWLEVEL_H_

extern void eink_display_update_full(u_int16_t mode);
extern void eink_display_update_full_area(u_int16_t mode, u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height);
extern void eink_display_update_part(u_int16_t mode);
extern void eink_display_update_part_area(u_int16_t mode, u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height);
extern int eink_display_raw_image(const unsigned char *data, unsigned int length);
extern int eink_display_packed_image(u_int16_t packmode, const unsigned char *data, unsigned int length);

extern void eink_init_rotmode(u_int16_t rotation_mode);
extern void eink_set_load_address(u_int32_t load_address);
extern void eink_set_display_address(u_int32_t display_address);

extern void eink_display_streamed_image_begin(u_int16_t packmode);
extern void eink_display_streamed_area_begin(u_int16_t packmode, u_int16_t xstart, u_int16_t ystart, u_int16_t width, u_int16_t height);
extern void eink_display_streamed_image_update(const unsigned char *data, unsigned int length);
extern int eink_display_streamed_image_end(void);

extern u_int16_t eink_read_register(u_int16_t reg);
extern void eink_write_register(const u_int16_t reg, const u_int16_t value);
extern int eink_perform_command(const u_int16_t command, 
		const u_int16_t * const params, const size_t params_len,
		u_int16_t * const response, const size_t response_len);
extern void eink_wait_display_idle(void);

extern int eink_controller_check_supported(void);

#endif /*EINK_LOWLEVEL_H_*/
