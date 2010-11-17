#include "config.h"

#include <usb.h>

#include <stdio.h>
#include "io.h"
#include "samba.h"

static usb_dev_handle *io_handle;

int io_init( char *dev )
{
  struct usb_bus *bus;
  struct usb_device *usbdev;
  
  usb_init(); /* initialize the library */
  usb_find_busses(); /* find all busses */
  usb_find_devices(); /* find all connected devices */

  for(bus = usb_get_busses(); bus; bus = bus->next) {
      for(usbdev = bus->devices; usbdev; usbdev = usbdev->next) {
          if(usbdev->descriptor.idVendor == USB_VID_ATMEL 
             && usbdev->descriptor.idProduct == USB_PID_SAMBA) {

	    
	    if( (io_handle = usb_open(usbdev)) ) {
	      if( usb_set_configuration( io_handle, 1 ) < 0 ){
		usb_close(io_handle);
		continue;
	      }
	      if( usb_claim_interface( io_handle, 1 ) < 0 ){
		usb_close(io_handle);
		continue;
	      }
	      return samba_init();

	    }
	  }
	  
      }
  }
  
  return -1;
}

int io_cleanup( void ) 
{
  usb_release_interface(io_handle, 1);
  usb_close(io_handle);
  return 0;
}


int io_write( void *buff, int len ) 
{
  int ret = usb_bulk_write(io_handle, 0x1, buff, len, 5000);
  if( ret < 0 ) {
    printf("error: %s\n", usb_strerror());
  }
  return ret;
}

int io_read( void *buff, int len ) 
{
  int ret = usb_bulk_read(io_handle, 0x82, buff, len, 5000);
  if( ret < 0 ) {
    printf("error: %s\n", usb_strerror());
  }
  return ret;
}
