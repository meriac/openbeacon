#ifndef __UI_H__
#define __UI_H__

// define pixel as 4 byte to make gfx operations faster on txtr.mini
typedef unsigned char TColor;
typedef unsigned short TPixelPos;

extern int ui_init (void);
extern void
ui_putpixel (TPixelPos x, TPixelPos y, TColor color)
  RAMFUNC;
     extern void ui_line (TPixelPos x0, TPixelPos y0, TPixelPos x1,
			  TPixelPos y1, TColor color);

#endif/*__UI_H__*/
