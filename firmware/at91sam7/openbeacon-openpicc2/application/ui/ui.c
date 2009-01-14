#include <FreeRTOS.h>
#include <string.h>
#include <board.h>
#include <stdlib.h>
#include <beacontypes.h>
#include "ui.h"

// define pixel as 4 byte to make gfx operations faster on txtr.mini
u_int32_t framebuffer[SCREEN_HEIGHT * SCREEN_WIDTH / 8]
  __attribute__ ((section (".sdram")));

#define SWAP_PIXELPOS(a,b) {TPixelPos tmp_pixel=a; a=b; b=tmp_pixel;}

RAMFUNC void
ui_putpixel (TPixelPos x, TPixelPos y, TColor color)
{
  u_int32_t *p;

  if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
    return;

  p = &framebuffer[(x / 8) + ((SCREEN_WIDTH / 8) * y)];
  x &= 0x7;
  *p = ((*p) & (0xF << x)) | (((u_int32_t) (color & 0x07)) << x);
}

void
ui_line (TPixelPos x0, TPixelPos y0, TPixelPos x1, TPixelPos y1, TColor color)
{
  int deltax, deltay, error, ystep, y, x;

  bool_t steep = abs (y1 - y0) > abs (x1 - x0);
  if (steep)
    {
      SWAP_PIXELPOS (x0, y0);
      SWAP_PIXELPOS (x1, y1);
    }

  if (x0 > x1)
    {
      SWAP_PIXELPOS (x0, x1);
      SWAP_PIXELPOS (y0, y1);
    }

  deltax = x1 - x0;
  deltay = abs (y1 - y0);
  error = deltax / 2;
  y = y0;
  ystep = (y0 < y1) ? 1 : -1;
  for (x = x0; x <= x1; x++)
    {
      if (steep)
	ui_putpixel (y, x, color);
      else
	ui_putpixel (x, y, color);

      error -= deltay;
      if (error < 0)
	{
	  y += ystep;
	  error += deltax;
	}
    }
}

static inline void
ui_tx_screendata (const void *four_bit_data, u_int16_t four_byte_blocks)
{
  // put actual transmission to display here
  (void) four_bit_data;
  (void) four_byte_blocks;
}

void
ui_flush (TPixelPos x, TPixelPos y, TPixelPos w, TPixelPos h)
{
  u_int32_t *p;

  if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
    return;

  if ((x + w) >= SCREEN_WIDTH)
    w = SCREEN_WIDTH - x;

  if ((y + h) >= SCREEN_HEIGHT)
    h = SCREEN_HEIGHT - y;

  // round to next eight pixel border
  w = ((x % 8) + w + 7) / 8;

  if(!w)
    return;
 
  p = &framebuffer[(y * (SCREEN_WIDTH / 8)) + (x / 8)];
  while (h--)
    {
      ui_tx_screendata (p, w);
      p += (SCREEN_WIDTH / 8);
    }
}

int
ui_init (void)
{
  memset (&framebuffer, sizeof (framebuffer), 0xFF);

  ui_line (0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 0x00);
  ui_line (SCREEN_WIDTH - 1, 0, 0, SCREEN_HEIGHT - 1, 0x00);
  ui_flush (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  return 0;
}
