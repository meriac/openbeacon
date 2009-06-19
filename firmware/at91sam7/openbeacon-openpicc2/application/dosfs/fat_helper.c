#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "eink/eink.h"
#include "dosfs/dosfs.h"

static VOLINFO vi;
static uint8_t __attribute__((section(".sdram"))) scratch[SECTOR_SIZE];
static uint8_t __attribute__((section(".sdram"))) sector[SECTOR_SIZE];

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

/* Read length bytes from filename into img_buf. Uses global scratch buffers, not reentrant. */
int fat_loadimage( uint8_t *filename, size_t length, eink_image_buffer_t img_buf ) {
	FILEINFO fi;
	int      res;
	uint32_t ro;
	size_t   remaining_read = length;
	
	if( !filename ) {
		return -EINVAL;
	}
	
	if( ( res = DFS_OpenFile ( &vi, filename, DFS_READ, scratch, &fi) ) != DFS_OK ) {
		printf( "Can't open file: %s, reason: %d\n", filename, res );
		return -res;
	}
	
	printf( "Opening file: %s, length: %lu\n", filename, fi.filelen );
	
	while(remaining_read > 0) {
		int to_read = remaining_read;
		if ( to_read > SECTOR_SIZE ) {
			to_read = SECTOR_SIZE;
		}
		
		res = DFS_ReadFile (&fi, scratch, sector, &ro, to_read);
		if( res != DFS_OK && res != DFS_EOF ) {
			return -res;
		}
		eink_image_buffer_load_stream(img_buf, sector, ro);
		
		if( res == DFS_EOF ) {
			break;
		}
		
		if ( ro == 0 ) {
			printf("Early end with %i to go\n", to_read);
			return -EIO;
		}
		remaining_read -= ro;
	}
	
	return 0;
}
