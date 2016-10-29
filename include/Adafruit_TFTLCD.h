// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#ifndef _ADAFRUIT_TFTLCD_H_
#define _ADAFRUIT_TFTLCD_H_

#include <stdarg.h>
#include <mraa/gpio.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Adafruit_GFX.h"

#define ILI9341_TFTWIDTH  	240
#define ILI9341_TFTHEIGHT 	320

// Color definitions
#define	ILI9341_BLACK   	0x0000
#define	ILI9341_BLUE    	0x001F
#define	ILI9341_RED     	0xF800
#define	ILI9341_GREEN   	0x07E0
#define ILI9341_CYAN    	0x07FF
#define ILI9341_MAGENTA 	0xF81F
#define ILI9341_YELLOW  	0xFFE0
#define ILI9341_WHITE   	0xFFFF

class Adafruit_TFTLCD : public Adafruit_GFX
{
public:

	Adafruit_TFTLCD(void);
	~Adafruit_TFTLCD(void);

	void begin(void);
	void end(void);
	void draw_pixel(int16_t x, int16_t y, uint16_t color);
	void draw_hline(int16_t x0, int16_t y0, int16_t w, uint16_t color);
	void draw_vline(int16_t x0, int16_t y0, int16_t h, uint16_t color);
	void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
	void fill_screen(uint16_t color);
	void reset(void);
	void set_registers8(uint8_t *ptr, uint8_t n);
	void set_registers16(uint16_t *ptr, uint8_t n);
	void set_rotation(uint8_t x);
	void set_addr_window(int x1, int y1, int x2, int y2);
	void push_colors(uint16_t *data, uint32_t len, bool first);

	uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
	uint16_t read_pixel(int16_t x, int16_t y);
	uint16_t read_ID(void);
	uint32_t read_reg(uint8_t r);

	void resize_image(int width, int height);
	void rotate_image(double angle);
	void draw_image(int x0, int y0);

	void set_image(cv::Mat img) { _img = img; }
	cv::Mat get_image(void) { return _img; }

	int get_err_num(void) const;
	bool is_open(void) const;
private:
	inline void write8(uint8_t value);
	inline void write16(uint16_t value);

	void set_wdir(void);
	void set_rdir(void);
	void write_register8(uint8_t a, uint8_t d);
	void write_register16(uint8_t a, uint16_t d);
	void write_register24(uint8_t a, uint32_t d);
	void write_register32(uint8_t a, uint32_t d);
	void write_register_pair(uint8_t aH, uint8_t aL, uint16_t d);

	void read8(uint8_t *result);

	void set_LR(void);
	void flood(uint16_t color, uint32_t len);

	mraa_gpio_context _cs;
	mraa_gpio_context _cd;
	mraa_gpio_context _wr;
	mraa_gpio_context _rd;
	mraa_gpio_context _data[8];

protected:
	int _err_num;
	bool _is_open;

	cv::Mat _img;
};

#endif
