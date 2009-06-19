#ifndef SDCARD_H_
#define SDCARD_H_

extern int DFS_Init(void);
extern int DFS_OpenCard(void);
extern u_int32_t DFS_ReadSector(u_int8_t unit, u_int8_t * buffer,
								u_int32_t sector, u_int32_t count);

#endif							/*SDCARD_H_ */
