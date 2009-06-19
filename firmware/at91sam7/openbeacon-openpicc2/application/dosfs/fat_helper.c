#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "eink/eink.h"
#include "dosfs/dosfs.h"
#include "utils.h"

static VOLINFO vi;
static uint8_t __attribute__((section(".sdram"))) scratch[SECTOR_SIZE];
static uint8_t __attribute__((section(".sdram"))) buffer[ROUND_UP(1024*1024,SECTOR_SIZE)];

/* Initialize a card, returns 0 on success. Uses global scratch buffer -> Not reentrant. */
int fat_init(void ) {
	uint32_t pstart, psize;
	int      res;
	uint8_t  pactive, ptype;
	
	if ((res = DFS_OpenCard ()) != 0) {
		printf ("Can't open SDCARD [%i]\n", res);
		return -ENODEV;
	}
	
	pstart = DFS_GetPtnStart (0, scratch, 0, &pactive, &ptype, &psize);
	if(pstart==0xffffffff) {
		printf ("Cannot find first partition\n");
		return -ENOENT;
	}
	
	if (DFS_GetVolInfo (0, scratch, pstart, &vi)) {
		printf ("Error getting volume information\n");
		return -EIO;
	}
	
	return 0;
}

static int _fat_loadimage( uint8_t *filename, size_t *length, eink_image_buffer_t img_buf, uint8_t *data_buf)
{
	FILEINFO fi;
	int      res;
	uint32_t ro;
	size_t   remaining_read = *length;
	
	/* *length will now contain the amount of data already read */
	*length = 0;
	
	if( !filename ) {
		return -EINVAL;
	}
	
	if ( !img_buf && !data_buf ) {
		return -EINVAL;
	}
	
	if( ( res = DFS_OpenFile ( &vi, filename, DFS_READ, scratch, &fi) ) != DFS_OK ) {
		printf( "Can't open file: %s, reason: %d\n", filename, res );
		return -res;
	}
	
	printf( "Opening file: %s, length: %lu\n", filename, fi.filelen );
	
	while(remaining_read > 0) {
		size_t to_read = remaining_read;
		uint8_t *target;
		if ( data_buf ) {
			target = data_buf + *length;
		} else {
			if ( to_read > sizeof(buffer) ) {
				to_read = sizeof(buffer);
			}
			target = buffer;
		}
		
		res = DFS_ReadFile (&fi, scratch, target, &ro, to_read);
		if( res != DFS_OK && res != DFS_EOF ) {
			return -res;
		}
		
		if( img_buf ) {
			eink_image_buffer_load_stream(img_buf, target, ro);
		}
		
		if( res == DFS_EOF ) {
			break;
		}
		
		*length += ro;
		remaining_read -= ro;
		
		if ( ro == 0 ) {
			printf("Early end with %i to go\n", to_read);
			return -EIO;
		}
	}
	
	return 0;
}

/* Read length bytes from filename into img_buf. Uses global scratch buffers, not reentrant.
 * Returns the number of actually read bytes in length */
int fat_load_eink_image_buffer(char *filename, size_t *length, eink_image_buffer_t img_buf )
{
	return _fat_loadimage((unsigned char*)filename, length, img_buf, NULL);
}

/* Read length bytes from filename into buffer. Uses global scratch buffers, not reentrant.
 * Returns the number of actually read bytes in length */
int fat_load_data_buffer(char *filename, size_t *length, uint8_t *buffer)
{
	return _fat_loadimage((unsigned char*)filename, length, NULL, buffer);
}
