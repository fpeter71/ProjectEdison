#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H

#include <stdint.h>

class Adafruit_GFX
{

public:

  Adafruit_GFX(int16_t w, int16_t h); // Constructor

  // This MUST be defined by the subclass:
  virtual void draw_pixel(int16_t x, int16_t y, uint16_t color) = 0;

  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void
	draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color),
	draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color),
	draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color),
	draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
	fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
	fill_screen(uint16_t color),
	invert_display(bool i);

  // These exist only with Adafruit_GFX (no subclass overrides)
  void
	draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
	draw_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color),
	fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
	fill_circle_helper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color),
	draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color),
	fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color),
	draw_round_rect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color),
	fill_round_rect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color),
	draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color),
	draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg),
	draw_xbitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color),
	draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size),
	set_cursor(int16_t x, int16_t y),
	set_text_color(uint16_t c),
	set_text_color(uint16_t c, uint16_t bg),
	set_text_size(uint8_t s),
	set_text_wrap(bool w),
	set_rotation(uint8_t r);

  virtual void write(uint8_t);

  void println(const char* string);

  int16_t height(void) const;
  int16_t width(void) const;

  uint8_t get_rotation(void) const;

protected:
	const int16_t WIDTH, HEIGHT;   // This is the 'raw' display w/h - never changes
	int16_t _width, _height, cursor_x, cursor_y;
	uint16_t textcolor, textbgcolor;
	uint8_t textsize, rotation;
	bool wrap; // If set, 'wrap' text at right edge of display
};

#endif // _ADAFRUIT_GFX_H
