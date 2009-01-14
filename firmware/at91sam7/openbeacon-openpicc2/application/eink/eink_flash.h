#ifndef EINK_FLASH_H_
#define EINK_FLASH_H_

extern int eink_flash_acquire(void);
extern int eink_flash_read(u_int32_t flash_address, char *dst, size_t len);
extern int eink_flash_write_enable(void);
extern int eink_flash_bulk_erase(void);
extern int eink_flash_read_identification(void);
extern int eink_flash_program_page(u_int32_t flash_address, const char *data, unsigned int len);
extern int eink_flash_program(const unsigned char *data, unsigned int len);
extern void eink_flash_release(void);
extern int eink_flash_conditional_reflash(void);

#endif /*EINK_FLASH_H_*/
