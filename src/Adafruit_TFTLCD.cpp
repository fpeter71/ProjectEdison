// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "Adafruit_TFTLCD.h"
#include "registers.h"

#define ARDUINO_BOARD	0

#if	ARDUINO_BOARD
	// Edison Arduino Pin numbers for the display connection
	static const int TFT_CS = 9;
	static const int TFT_CD = 8;
	static const int TFT_WR = 7;
	static const int TFT_RD = 6;
	static const int TFT_DATA[8] = { 13, 10, 12, 11, 14, 15, 16, 17 };
#else
	static const int TFT_CS = 21;
	static const int TFT_CD = 47;
	static const int TFT_WR = 33;
	static const int TFT_RD = 0;
	static const int TFT_DATA[8] = { 37, 51, 50, 38, 31, 45, 32, 46 };
#endif

// First hardware pin number of the data bits, ie 40-47, and WR flag assumed in the same % 32 range
static const int TFT_EDISON_DATA0 = 40;
static const int TFT_EDISON_WR = 48;

#define TFTWIDTH   			240
#define TFTHEIGHT  			320

#define HIGH                1
#define LOW                 0

// When using the TFT breakout board, control pins are configurable.
#define RD_LOW  	mraa_gpio_write(_rd, LOW)
#define RD_HIGH    	mraa_gpio_write(_rd, HIGH)
#define WR_LOW  	mraa_gpio_write(_wr, LOW)
#define WR_HIGH    	mraa_gpio_write(_wr, HIGH)
#define CD_COMMAND 	mraa_gpio_write(_cd, LOW)
#define CD_DATA    	mraa_gpio_write(_cd, HIGH)
#define CS_LOW  	mraa_gpio_write(_cs, LOW)
#define CS_HIGH    	mraa_gpio_write(_cs, HIGH)

// Data write strobe, ~2 instructions and always inline
//#define WR_STROBE { WR_LOW; WR_HIGH; }
#define WR_STROBE { uint32_t bit = (uint32_t)((uint32_t)1 << (TFT_EDISON_WR % 32)); \
	*(volatile uint32_t*)mmap_reg_clear = bit; \
	*(volatile uint32_t*)mmap_reg_set = bit; }

static void delaym(int microseconds)
{
	usleep(microseconds * 1000);
}

static void delayu(int milliseconds)
{
	usleep(milliseconds);
}

// This is an absolute path to a resource file found within sysfs.
// Might not always be correct. First thing to check if mmap stops
// working. Check the device for 0x1199 and Intel Vendor (0x8086)
#define MMAP_PATH "/sys/devices/pci0000:00/0000:00:0c.0/resource0"

//MMAP
static uint8_t *mmap_reg = NULL;
static uint8_t *mmap_reg_set = NULL;
static uint8_t *mmap_reg_clear = NULL;
static int mmap_fd = 0;
static int mmap_size;

// Constructor for breakout board (configurable LCD control lines).
Adafruit_TFTLCD::Adafruit_TFTLCD() : Adafruit_GFX(TFTWIDTH, TFTHEIGHT)
{
	_is_open = false;

	_cs = mraa_gpio_init(TFT_CS);
	if (_cs == NULL) {
		_err_num = 2;
		return;
	}
	_cd = mraa_gpio_init(TFT_CD);
	if (_cd == NULL) {
		_err_num = 3;
		return;
	}
	_wr = mraa_gpio_init(TFT_WR);
	if (_wr == NULL) {
		_err_num = 4;
		return;
	}
	_rd = mraa_gpio_init(TFT_RD);
	if (_rd == NULL) {
		_err_num = 5;
		return;
	}
	for (int i = 0; i < 8; ++i)
	{
		_data[i] = mraa_gpio_init(TFT_DATA[i]);
		if (_data[i] == NULL) {
			_err_num = 6 + i;
			return;
		}

		mraa_gpio_use_mmaped(_data[i], 1);
	}

	mraa_gpio_dir(_cs, MRAA_GPIO_OUT);
	mraa_gpio_dir(_cd, MRAA_GPIO_OUT);
	mraa_gpio_dir(_wr, MRAA_GPIO_OUT);
	mraa_gpio_dir(_rd, MRAA_GPIO_OUT);

	mraa_gpio_use_mmaped(_cs, 1);
	mraa_gpio_use_mmaped(_cd, 1);
	mraa_gpio_use_mmaped(_wr, 1);
	mraa_gpio_use_mmaped(_rd, 1);

    if (mmap_reg == NULL) {
        if ((mmap_fd = open(MMAP_PATH, O_RDWR)) < 0) {
        	_err_num = 20;
        	return;
        }

        struct stat fd_stat;
        fstat(mmap_fd, &fd_stat);
        mmap_size = fd_stat.st_size;

        mmap_reg = (uint8_t*) mmap(NULL, fd_stat.st_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, mmap_fd, 0);
        if (mmap_reg == MAP_FAILED) {
            mmap_reg = NULL;
            close(mmap_fd);
            _err_num = 21;
            return;
        }

        uint8_t offset = ((TFT_EDISON_DATA0 / 32) * sizeof(uint32_t));
        mmap_reg_set = mmap_reg + offset + 0x34;
        mmap_reg_clear = mmap_reg + offset + 0x4c;
    }

	CS_HIGH; // Set all control bits to idle state
	WR_HIGH;
	RD_HIGH;
	CD_DATA;

	set_wdir(); // Set up LCD data port(s) for WRITE operations

	rotation  = 0;
	cursor_y  = cursor_x = 0;
	textsize  = 1;
	textcolor = 0xFFFF;
	_width    = TFTWIDTH;
	_height   = TFTHEIGHT;

	_is_open = true;
}

Adafruit_TFTLCD::~Adafruit_TFTLCD(void)
{
	if(_is_open){
		end();
	}
}

void Adafruit_TFTLCD::begin(void)
{
	reset();

	delaym(200);

	CS_LOW;
	write_register8(ILI9341_SOFTRESET, 0);
	delaym(50);
	write_register8(ILI9341_DISPLAYOFF, 0);

	write_register8(ILI9341_POWERCONTROL1, 0x23);
	write_register8(ILI9341_POWERCONTROL2, 0x10);
	write_register16(ILI9341_VCOMCONTROL1, 0x2B2B);
	write_register8(ILI9341_VCOMCONTROL2, 0xC0);
	write_register8(ILI9341_MEMCONTROL, ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
	write_register8(ILI9341_PIXELFORMAT, 0x55);
	write_register16(ILI9341_FRAMECONTROL, 0x001B);

	write_register8(ILI9341_ENTRYMODE, 0x07);

	write_register8(ILI9341_SLEEPOUT, 0);
	delaym(150);
	write_register8(ILI9341_DISPLAYON, 0);
	delaym(500);
	set_addr_window(0, 0, TFTWIDTH-1, TFTHEIGHT-1);

	_is_open = true;
}

void Adafruit_TFTLCD::end(void)
{
	CS_LOW; // Set all control bits to idle state
	WR_LOW;
	RD_LOW;
	CD_COMMAND;

	write8(0x00);

	mraa_gpio_close(_cs);
	mraa_gpio_close(_wr);
	mraa_gpio_close(_rd);
	mraa_gpio_close(_cd);

	for (int i = 0; i < 8; ++i)
	{
		mraa_gpio_close(_data[i]);
	}

	_is_open = false;
}

void Adafruit_TFTLCD::reset(void)
{
	CS_HIGH;
	WR_HIGH;
	RD_HIGH;

	// Data transfer sync
	CS_LOW;
	CD_COMMAND;
	write8(0x00);
	for(uint8_t i = 0; i < 3; i++)
		WR_STROBE; // Three extra 0x00s

	CS_HIGH;
}

// Sets the LCD address window (and address counter, on 932X).
// Relevant to rect/screen fills and H/V lines.  Input coordinates are
// assumed pre-sorted (e.g. x2 >= x1).
void Adafruit_TFTLCD::set_addr_window(int x1, int y1, int x2, int y2)
{
	CS_LOW;

	uint32_t t;

	t = x1;
	t <<= 16;
	t |= x2;
	write_register32(ILI9341_COLADDRSET, t);
	t = y1;
	t <<= 16;
	t |= y2;
	write_register32(ILI9341_PAGEADDRSET, t);

	CS_HIGH;
}

// Unlike the 932X drivers that set the address window to the full screen
// by default (using the address counter for draw_pixel operations), the
// 7575 needs the address window set on all graphics operations.  In order
// to save a few register writes on each pixel drawn, the lower-right
// corner of the address window is reset after most fill operations, so
// that draw_pixel only needs to change the upper left each time.
void Adafruit_TFTLCD::set_LR(void)
{
	CS_LOW;
	write_register_pair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, _width  - 1);
	write_register_pair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO, _height - 1);
	CS_HIGH;
}

// Fast block fill operation for fill_screen, fill_rect, H/V line, etc.
// Requires set_addr_window() has previously been called to set the fill
// bounds.  'len' is inclusive, MUST be >= 1.
void Adafruit_TFTLCD::flood(uint16_t color, uint32_t len)
{
	int blocks;
	int i;
	uint8_t hi = color >> 8,
			lo = color;

	CS_LOW;
	CD_COMMAND;

	write8(0x2C);

	CD_DATA;

	if(hi == lo) {
		// Write first pixel normally, decrement counter by 1
		write16(color);
		len--;

		blocks = (uint16_t)(len / 64); // 64 pixels/block

		// High and low bytes are identical.  Leave prior data
		// on the port(s) and just toggle the write strobe.
		while(blocks--) {
			i = 16; // 64 pixels/block / 4 pixels/pass
			do {
				WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // 2 bytes/pixel
				WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // x 4 pixels
			} while(--i);
		}
		// Fill any remaining pixels (1 to 64)
		for(i = (uint8_t)len & 63; i--; ) {
			WR_STROBE;
			WR_STROBE;
		}
	}
	else
	{
		for(int l = len; l; --l) {
			write16(color);
		}
	}

	CS_HIGH;
}

void Adafruit_TFTLCD::draw_hline(int16_t x, int16_t y, int16_t length, uint16_t color)
{
	int16_t x2;

	// Initial off-screen clipping
	if((length <= 0) || (y < 0) || (y >= _height) || (x >= _width) || ((x2 = (x + length-1)) < 0))
		return;

	if(x < 0) {        // Clip left
		length += x;
		x = 0;
	}

	if(x2 >= _width) { // Clip right
		x2 = _width - 1;
		length = x2 - x + 1;
	}

	set_addr_window(x, y, x2, y);
	flood(color, length);
	set_LR();
}

void Adafruit_TFTLCD::draw_vline(int16_t x, int16_t y, int16_t length, uint16_t color)
{
	int16_t y2;

	// Initial off-screen clipping
	if((length <= 0) || (x <  0) || (x >= _width) || (y>= _height) || ((y2 = (y+length-1)) < 0))
		return;

	if(y < 0) {         // Clip top
		length += y;
		y = 0;
	}
	if(y2 >= _height) { // Clip bottom
		y2 = _height - 1;
		length  = y2 - y + 1;
	}

	set_addr_window(x, y, x, y2);
	flood(color, length);
	set_LR();
}

void Adafruit_TFTLCD::fill_rect(int16_t x1, int16_t y1, int16_t w, int16_t h,  uint16_t fillcolor)
{
	int16_t  x2, y2;

	// Initial off-screen clipping
	if((w <= 0 ) || (h <= 0) || (x1 >= _width) || (y1 >= _height) || ((x2 = x1+w-1) < 0) || ((y2 = y1+h-1) < 0))
		return;

	if(x1 < 0) { // Clip left
		w += x1;
		x1 = 0;
	}

	if(y1 < 0) { // Clip top
		h += y1;
		y1 = 0;
	}

	if(x2 >= _width) { // Clip right
		x2 = _width - 1;
		w  = x2 - x1 + 1;
	}

	if(y2 >= _height) { // Clip bottom
		y2 = _height - 1;
		h  = y2 - y1 + 1;
	}

	set_addr_window(x1, y1, x2, y2);
	flood(fillcolor, (uint32_t)w * (uint32_t)h);
	set_LR();
}

void Adafruit_TFTLCD::fill_screen(uint16_t color)
{
	// For these, there is no settable address pointer, instead the
	// address window must be set for each drawing operation.  However,
	// this display takes rotation into account for the parameters, no
	// need to do extra rotation math here.
	set_addr_window(0, 0, _width - 1, _height - 1);

	flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

void Adafruit_TFTLCD::draw_pixel(int16_t x, int16_t y, uint16_t color)
{
	// Clip
	if((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
		return;

	CS_LOW;
	set_addr_window(x, y, _width-1, _height-1);
	CS_LOW;
	CD_COMMAND;
	write8(0x2C);
	CD_DATA;
	write8(color >> 8);
	write8(color);

	CS_HIGH;
}

// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
void Adafruit_TFTLCD::push_colors(uint16_t *data, uint32_t len, bool first)
{
	CS_LOW;
	if(first == true) { // Issue GRAM write command only on first call
		CD_COMMAND;
		write8(0x2C);
	}

	CD_DATA;
	while(len--) {
		uint16_t color = *data++;
		write16(color);
	}
	CS_HIGH;
}

void Adafruit_TFTLCD::set_rotation(uint8_t x)
{
	// Call parent rotation func first -- sets up rotation flags, etc.
	Adafruit_GFX::set_rotation(x);
	// Then perform hardware-specific rotation operations...

	CS_LOW;
	// MEME, HX8357D uses same registers as 9341 but different values
	uint16_t t = 0;
	switch (rotation) {
		case 2:
			t = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
			break;
		case 3:
			t = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
			break;
		case 0:
			t = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
			break;
		case 1:
			t = ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR;
			break;
	}
	write_register8(ILI9341_MADCTL, t ); // MADCTL
	// For 9341, init default full-screen address window:
	set_addr_window(0, 0, _width - 1, _height - 1); // CS_IDLE happens here
}


// Because this function is used infrequently, it configures the ports for
// the read operation, reads the data, then restores the ports to the write
// configuration.  Write operations happen a LOT, so it's advantageous to
// leave the ports in that state as a default.
uint16_t Adafruit_TFTLCD::read_pixel(int16_t x, int16_t y)
{
	if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return 0;

	CS_LOW;
	return 0;
}

// Ditto with the read/write port directions, as above.
uint16_t Adafruit_TFTLCD::read_ID(void)
{
	uint8_t p1, p2, p3, p4;
	uint16_t id;

	CS_LOW;
	RD_HIGH;
	WR_HIGH;
	CD_COMMAND;
	write8(0x04);
	set_rdir();  // Set up LCD data port(s) for READ operations
	CD_DATA;
	delayu(50);
	read8(&p1);
	read8(&p2);
	read8(&p3);
	read8(&p4);
	set_wdir();  // Restore LCD data port(s) to WRITE configuration
	CS_HIGH;
	RD_LOW;
	WR_LOW;

	printf("0x%x\n", p2);
	printf("0x%x\n", p3);
	printf("0x%x\n", p4);

	id = p3 << 8;
	id |= p4;
	return id;
}

uint32_t Adafruit_TFTLCD::read_reg(uint8_t r)
{
	uint32_t id;
	uint8_t x;

	// try reading register #4
	CS_LOW;
	CD_COMMAND;
	write8(r);
	set_rdir();  // Set up LCD data port(s) for READ operations
	CD_DATA;
	delayu(50);
	read8(&x);
	id = x;          // Do not merge or otherwise simplify
	id <<= 8;              // these lines.  It's an unfortunate
	read8(&x);
	id  |= x;        // shenanigans that are going on.
	id <<= 8;              // these lines.  It's an unfortunate
	read8(&x);
	id  |= x;        // shenanigans that are going on.
	id <<= 8;              // these lines.  It's an unfortunate
	read8(&x);
	id  |= x;        // shenanigans that are going on.
	CS_HIGH;
	set_wdir();  // Restore LCD data port(s) to WRITE configuration

	return id;
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t Adafruit_TFTLCD::color565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// For I/O macros that were left undefined, declare function
// versions that reference the inline macros just once:

void Adafruit_TFTLCD::write8(uint8_t value)
{
    uint32_t set   = (uint32_t)value << (TFT_EDISON_DATA0 % 32);
    uint32_t clear = ((uint32_t)((uint8_t)~value) << (TFT_EDISON_DATA0 % 32)) | (uint32_t)1u << (TFT_EDISON_WR % 32);
    uint32_t setWr   = (uint32_t)1 << (TFT_EDISON_WR % 32);

    *(volatile uint32_t*)mmap_reg_set = set;
    *(volatile uint32_t*)mmap_reg_clear = clear;
    *(volatile uint32_t*)mmap_reg_set = setWr;
}

void Adafruit_TFTLCD::write16(uint16_t value)
{
	uint8_t hi = value >> 8;
	uint8_t lo = value;

	uint32_t setHi   = (uint32_t)hi << (TFT_EDISON_DATA0 % 32);
	uint32_t clearHi = ((uint32_t)((uint8_t)~hi) << (TFT_EDISON_DATA0 % 32)) | (uint32_t)1 << (TFT_EDISON_WR % 32);
	uint32_t setLo   = (uint32_t)lo << (TFT_EDISON_DATA0 % 32);
	uint32_t clearLo = ((uint32_t)((uint8_t)~lo) << (TFT_EDISON_DATA0 % 32)) | (uint32_t)1 << (TFT_EDISON_WR % 32);
	uint32_t setWr   = (uint32_t)1 << (TFT_EDISON_WR % 32);

	*(volatile uint32_t*)mmap_reg_set = setHi;
	*(volatile uint32_t*)mmap_reg_clear = clearHi;
	*(volatile uint32_t*)mmap_reg_set = setWr;

	*(volatile uint32_t*)mmap_reg_set = setLo;
	*(volatile uint32_t*)mmap_reg_clear = clearLo;
	*(volatile uint32_t*)mmap_reg_set = setWr;
}

void Adafruit_TFTLCD::read8(uint8_t *result) {
	RD_LOW;

	// Sleep minimum of 400 ns
	timespec step;
	step.tv_sec = 0;
	step.tv_nsec = 400;
	nanosleep(&step, NULL);

	*result = 0;
	for (int i = 0; i < 8; ++i)
	{
		*result |= mraa_gpio_read(_data[i]) >> i;
	}

	RD_HIGH;
}


void Adafruit_TFTLCD::set_wdir(void)
{
    for (int i = 0; i < 8; ++i)
	{
		mraa_gpio_dir(_data[i], MRAA_GPIO_OUT);
	}
}

void Adafruit_TFTLCD::set_rdir(void)
{
    for (int i = 0; i < 8; ++i)
	{
		mraa_gpio_dir(_data[i], MRAA_GPIO_IN);
	}
}

void Adafruit_TFTLCD::write_register8(uint8_t a, uint8_t d)
{
	CD_COMMAND;
	write8(a);
	CD_DATA;
	write8(d);
}

void Adafruit_TFTLCD::write_register16(uint8_t a, uint16_t d)
{
	CD_COMMAND;
	write8(a);
	CD_DATA;
	write16(d);
}

void Adafruit_TFTLCD::write_register_pair(uint8_t aH, uint8_t aL, uint16_t d)
{
	// Set value of 2 TFT registers: Two 8-bit addresses (hi & lo), 16-bit value
	uint8_t hi = (d) >> 8, lo = (d);
	CD_COMMAND;
	write8(aH);
	CD_DATA;
	write8(hi);
	CD_COMMAND;
	write8(aL);
	CD_DATA;
	write8(lo);
}

void Adafruit_TFTLCD::write_register24(uint8_t r, uint32_t d)
{
	CS_LOW;
	CD_COMMAND;
	write8(r);
	CD_DATA;
	delayu(10);
	write8(d >> 16);
	delayu(10);
	write8(d >> 8);
	delayu(10);
	write8(d);
	CS_HIGH;
}

void Adafruit_TFTLCD::write_register32(uint8_t r, uint32_t d)
{
	CS_LOW;
	CD_COMMAND;
	write8(r);
	CD_DATA;
	delayu(10);
	write8(d >> 24);
	delayu(10);
	write8(d >> 16);
	delayu(10);
	write8(d >> 8);
	delayu(10);
	write8(d);
	CS_HIGH;
}

int Adafruit_TFTLCD::get_err_num(void) const
{
	return _err_num;
}

bool Adafruit_TFTLCD::is_open(void) const
{
	return _is_open;
}

void Adafruit_TFTLCD::resize_image(int width, int height)
{
	cv::Mat ret;
	cv::Size size(width, height);
	cv::resize(_img, ret, size);
	_img = ret;
}

void Adafruit_TFTLCD::rotate_image(double angle)
{
	cv::Mat ret;
	cv::Point2f pt(_img.cols/2.0, _img.rows/2.0);
	cv::Mat rot = cv::getRotationMatrix2D(pt, angle, 1.0);
	cv::Rect bbox = cv::RotatedRect(pt, _img.size(), angle).boundingRect();
	rot.at<double>(0,2) += bbox.width/2.0 - pt.x;
	rot.at<double>(1,2) += bbox.height/2.0 - pt.y;
	cv::warpAffine(_img, ret, rot, bbox.size());
	_img = ret;
}

void Adafruit_TFTLCD::draw_image(int x0, int y0)
{
	int x, y, k = 0;
	uint8_t r, g, b;
	cv::Mat canv(_width, _height, CV_8UC3);
	uint16_t img16[_width * _height];

	memset(img16, 0, (_width * _height) / 2 );

	int x0_rot = y0;
	int y0_rot = _width - x0 - _img.rows;

	for(y = 0; y < _img.rows; y++){
		for(x = 0; x < _img.cols; x++){
			cv::Vec3b color = _img.at<cv::Vec3b>(cv::Point(x, y));
			canv.at<cv::Vec3b>(cv::Point(x0_rot + x, y0_rot + y)) = color;
		}
	}

	for(y = 0; y < canv.rows; y++){
		for(x = 0; x < canv.cols; x++){
			cv::Vec3b color = canv.at<cv::Vec3b>(cv::Point(x, y));

			b = color[0];
			g = color[1];
			r = color[2];

			img16[k++] = color565(r, g, b);
		}
	}

	set_rotation(1);

	push_colors(img16, canv.cols * canv.rows, true);

	set_rotation(2);
}
