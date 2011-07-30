#include <AT91SAM7.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <USB-CDC.h>
#include <board.h>
#include <beacontypes.h>
#include <dbgu.h>
#include <env.h>

#include "proto.h"
#include "cmd.h"
#include "nRF24L01/nRF_API.h"
#include "nRF24L01/nRF_HW.h"
#include "led.h"
#include "nRF24L01/nRF_CMD.h"

xQueueHandle xCmdQueue;
xTaskHandle xCmdTask;
xTaskHandle xCmdRecvUsbTask;
/**********************************************************************/

char rs232_buffer[1024];
unsigned int rs232_buffer_pos;

void vSendByte(char data)
{
    if(rs232_buffer_pos<sizeof(rs232_buffer))
	rs232_buffer[rs232_buffer_pos++]=data;
	
    if(data=='\r')
    {
	AT91F_DBGU_Frame(rs232_buffer,rs232_buffer_pos);
	rs232_buffer_pos=0;
    }
    
    vUSBSendByte(data);
}

void DumpUIntToUSB(unsigned int data)
{
    int i=0;
    unsigned char buffer[10],*p=&buffer[sizeof(buffer)];

    do {
        *--p='0'+(unsigned char)(data%10);
        data/=10;
        i++;
    } while(data);

    while(i--)
        vSendByte(*p++);
}
/**********************************************************************/

void DumpStringToUSB(char* text)
{
    unsigned char data;

    if(text)
        while((data=*text++)!=0)
            vSendByte(data);
}
/**********************************************************************/

static inline unsigned char HexChar(unsigned char nibble)
{
   return nibble + ((nibble<0x0A) ? '0':('A'-0xA));
}
/**********************************************************************/

void DumpBufferToUSB(char* buffer, int len)
{
    int i;

    for(i=0; i<len; i++) {
	vSendByte(HexChar( *buffer  >>   4));
	vSendByte(HexChar( *buffer++ & 0xf));
    }
}
/**********************************************************************/

int atoiEx(const char * nptr)
{
	int sign=1, i=0, curval=0;

	while(nptr[i] == ' ' && nptr[i] != 0)
	    i++;
	    
	if(nptr[i] != 0)
	{
	    if(nptr[i] == '-')
	    {
		sign *= -1;
		i++;
	    }
	    else
		if(nptr[i] == '+')
		    i++;
	    
	    while(nptr[i] != 0 && nptr[i] >= '0' && nptr[i] <= '9')
		curval = curval * 10 + (nptr[i++] - '0');
	}
	
	return sign * curval;
}
/**********************************************************************/

void prvExecCommand(u_int32_t cmd, portCHAR *args) {
	int i,h,m,s;		    
	i=0;

	switch(cmd)
	{
		case '4':
		case '3':
		case '2':
		case '1':
		    env.e.mode=cmd-'0';
		    DumpStringToUSB(" * Switched to transmit mode at power level ");
		    DumpUIntToUSB(env.e.mode);
		    DumpStringToUSB("\n\r");
	    	    break;
		case '0':
		    DumpStringToUSB(" * switched to RX mode\n\r");		    
		    break;
		case 'S':
		    env_store();
		    DumpStringToUSB(" * Stored environment variables\n\r");
		    break;
		case 'FIFO':
		    i=PtSetFifoLifetimeSeconds(atoiEx(args));
		    if(i<0)
			DumpStringToUSB(" * Invalid FIFO cache lifetime parameter\n\r");
		    else
		    {
			DumpStringToUSB("FIFO lifetime set to ");
			DumpUIntToUSB(i);
			DumpStringToUSB("\n\r");
		    }			
		    break;
		case 'I':
		    i=atoiEx(args);
		    if(i!=0) {
			env.e.reader_id = i;
			DumpStringToUSB("Reader ID set to ");
			DumpUIntToUSB(env.e.reader_id);
			DumpStringToUSB("\n\r");
		    }
		    break;
		case 'C':
		    DumpStringToUSB(
			" *****************************************************\n\r"
			" * Current configuration:                            *\n\r"
			" *****************************************************\n\r"
			" *\n\r");
		    DumpStringToUSB(" * Uptime is ");
		    s=xTaskGetTickCount()/1000;
		    h=s/3600;
		    s%=3600;
		    m=s/60;
		    s%=60;
		    DumpUIntToUSB(h);
		    DumpStringToUSB("h:");
		    DumpUIntToUSB(m);
		    DumpStringToUSB("m:");
		    DumpUIntToUSB(s);
		    DumpStringToUSB("s");
		    DumpStringToUSB("\n\r");
		    DumpStringToUSB(" * The reader id is ");
		    DumpUIntToUSB(env.e.reader_id);
		    DumpStringToUSB("\n\r");
		    DumpStringToUSB(" * The mode is ");
		    DumpUIntToUSB(env.e.mode);
		    DumpStringToUSB("\n\r");
		    DumpStringToUSB(" * The channel is ");
		    DumpUIntToUSB(nRFAPI_GetChannel(DEFAULT_DEV));
		    DumpStringToUSB("\n\r");
		    DumpStringToUSB(" * The FIFO cache lifetime is ");
		    DumpUIntToUSB(PtGetFifoLifetimeSeconds());
		    DumpStringToUSB("s\n\r");
		    DumpStringToUSB(
			" *\n\r"
			" *****************************************************\n\r"
			);
		    break;
		case 'H':	
		case '?':
		    DumpStringToUSB(
			" *****************************************************\n\r"
			" * OpenBeacon USB terminal                           *\n\r"
			" * (C) 2007 Milosch Meriac <meriac@openbeacon.de>    *\n\r"
			" *****************************************************\n\r"
			" *\n\r"
			" * S          - store transmitter settings\n\r"
			" * C          - print configuration\n\r"
			" * I [id]     - set reader id\n\r"
			" * FIFO [sec] - set FIFO cache lifetime in seconds\n\r"
			" * 0          - receive only mode\n\r"
			" * 1..4       - automatic transmit at selected power levels\n\r"
			" * ?,H        - display this help screen\n\r"
			" *\n\r"
			" *****************************************************\n\r"
			);
		    break;
	    }
    
}
/**********************************************************************/

// A task to execute commands
void vCmdCode(void *pvParameters) {
	(void) pvParameters;
	u_int32_t cmd;
	portBASE_TYPE i, j=0;
	
	for(;;) {
		cmd_type next_command;
		cmd = j = 0;
		 if( xQueueReceive(xCmdQueue, &next_command, ( portTickType ) 100 ) ) {
			DumpStringToUSB("Command received:");
			DumpStringToUSB(next_command.command);
			DumpStringToUSB("\n\r");
			while(next_command.command[j] == ' ' && next_command.command[j] != 0) {
				j++;
			}
			for(i=0;i<4;i++) {
				portCHAR cByte = next_command.command[i+j];
				if(next_command.command[i+j] == 0 || next_command.command[i+j] == ' ')
					break;
				if(cByte>='a' && cByte<='z') {
					cmd = (cmd<<8) | (cByte+('A'-'a'));
				} else cmd = (cmd<<8) | cByte;
			}
			while(next_command.command[i+j] == ' ' && next_command.command[i+j] != 0) {
				i++;
			}
			prvExecCommand(cmd, next_command.command+i+j);
		 } else {
		 }
	}
}
/**********************************************************************/

// A task to read commands from USB
void vCmdRecvUsbCode(void *pvParameters) {
	portBASE_TYPE len=0;
	cmd_type next_command = { source: SRC_USB, command: ""};
	(void) pvParameters;
    
	for( ;; ) {
		if(vUSBRecvByte(&next_command.command[len], 1, 100)) {
			next_command.command[len+1] = 0;
			DumpStringToUSB(next_command.command + len);
			if(next_command.command[len] == '\n' || next_command.command[len] == '\r') {
				next_command.command[len] = 0;
				if(len > 0) {
			    		if( xQueueSend(xCmdQueue, &next_command, 0) != pdTRUE) {
			    			DumpStringToUSB("Queue full, command can't be processed.\n");
			    		}
			    		len=0;
				}
			} else len++;
			if(len >= MAX_CMD_LEN-1) {
				DumpStringToUSB("ERROR: Command too long. Ignored.");
				len=0;
			}
	    	}
    	}
}

portBASE_TYPE vCmdInit(void) {

	rs232_buffer_pos=0;

	/* FIXME Maybe modify to use pointers? */
	xCmdQueue = xQueueCreate( 10, sizeof(cmd_type) );
	if(xCmdQueue == 0) {
		return 0;
	}
	
	if(xTaskCreate(vCmdCode, (signed portCHAR *)"CMD", TASK_CMD_STACK, NULL, 
		TASK_CMD_PRIORITY, &xCmdTask) != pdPASS) {
		return 0;
	}
	
	if(xTaskCreate(vCmdRecvUsbCode, (signed portCHAR *)"CMDUSB", TASK_CMD_STACK, NULL, 
		TASK_CMD_PRIORITY, &xCmdRecvUsbTask) != pdPASS) {
		return 0;
	}
	
	return 1;
}
