#ifndef SDCARD_H_
#define SDCARD_H_

extern int sdcard_open_card(void);
extern int sdcard_init(void);
extern int sdcard_disk_read (u_int8_t * buff,u_int32_t sector,u_int32_t count);

#endif /*SDCARD_H_*/
