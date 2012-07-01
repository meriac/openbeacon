#ifndef CMD_H_
#define CMD_H_

#include <queue.h>

#define MAX_CMD_LEN 32

typedef struct
{
	enum
	{ SRC_USB, SRC_RF } source;
	portCHAR command[MAX_CMD_LEN];
} cmd_type;

portBASE_TYPE vCmdInit (void);
extern void DumpUIntToUSB (unsigned int data);
extern void DumpStringToUSB (char *string);
extern void DumpBufferToUSB (char *buffer, int len);
extern xQueueHandle xCmdQueue;

#endif /*CMD_H_ */
