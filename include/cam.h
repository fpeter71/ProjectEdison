#ifndef __CAM_H__
#define __CAM_H__

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include d<stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "opencv2/opencv.hpp"

#define CAM_SUCCESS		0
#define CAM_EOPEN		1
#define CAM_EREQBUFF	2
#define CAM_EQUERYBUF	3
#define CAM_EMMAP		4
#define CAM_EQUERYCAP	20
#define CAM_ECAPCAPTURE	21
#define CAM_ESETFMT		22
#define CAM_EEXPOSUREM	23
#define CAM_EEXPOSURET	24
#define CAM_EAFOCUS		25
#define CAM_EFPS1		26
#define CAM_EFPS2		27
#define CAM_EQBUFSS		50
#define CAM_ESTREAMON	51
#define CAM_ESTREAMOFF	52
#define CAM_ESTREAMST	53
#define CAM_EINFO1		60
#define CAM_EINFO2		61
#define CAM_EDQBUF1		90
#define CAM_EDQBUF2		91
#define CAM_ENULLIMG	92

#define CAM_CAPTURE_FAST	1
#define CAM_CAPTURE_NORMAL	2

struct buffer {
	void   *start;
    size_t  length;
};

namespace etm {

	class Cam {
private:
		int _fd;
		uint32_t _width;
		uint32_t _height;
		uint32_t _fps;
		char _pixel_format[5];
		bool _is_open;
		bool _is_capture_ok;
		int _err_num;
		struct buffer  *_buffers;
		uint32_t _n_buffers;
		uint32_t _buffer_size;
		bool _is_streaming;

		int xioctl(int request, void *arg);
		bool init_dev(void);
		bool init_mmap(void);
		bool uninit_mmap(void);

		void memcpy2a(uint8_t *dest, const uint8_t *src, ssize_t length);

public:
		Cam(void);
		Cam(char *name, uint32_t width, uint32_t height, char *pixel_format, uint32_t fps);
		~Cam(void);

		bool start_stream(void);
		bool stop_stream(void);

		void capture_image(uint8_t *image, uint8_t mode);
		void print_info(void);

		bool is_open(void) const { return _is_open; };
		bool is_capture_ok(void) const { return _is_capture_ok; };

		int get_err_num(void) const { return _err_num; };
		ssize_t get_buffer_size(void) const { return _buffer_size; };
		uint32_t get_fps(void) const { return _fps; };
		uint32_t get_width(void) const { return _width; };
		uint32_t get_height(void) const { return _height; };
		void get_pixel_format(char *pxf) const;
	};
}

#endif
