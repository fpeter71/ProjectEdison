#include "cam.h"

etm::Cam::Cam(char *name, uint32_t width, uint32_t height, char *pixel_format, uint32_t fps)
{
	_width = width;
	_height = height;
	_fps = fps;
	_n_buffers = 4;

	strcpy(_pixel_format, pixel_format);

	_is_open = true;
	_err_num = 0;

	_is_streaming = false;

	_fd = open(name, O_RDWR | O_NONBLOCK);
	if (_fd == -1){
		_is_open = false;
		_err_num = CAM_EOPEN;
		return;
	}

	_is_open &= init_dev();

	if(!_is_open)
		return;

	_is_open &= init_mmap();
}

etm::Cam::Cam(void)
{
	_width = 0;
	_height = 0;
	_err_num = 0;
	_is_streaming = false;

	strcpy(_pixel_format, "NONE");

	_fd = -1;
	_is_open = false;
}

etm::Cam::~Cam(void)
{
	stop_stream();

	uninit_mmap();

	if(_is_open || _fd > 0){
		close(_fd);
	}
}

int etm::Cam::xioctl(int request, void *arg)
{
	int ret = -1;
	if(_is_open){
		do ret = ioctl (_fd, request, arg);
		while (ret == -1 && EINTR == errno);
	}
	return ret;
}

bool etm::Cam::init_mmap(void)
{
	uint8_t n;

	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count = _n_buffers;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (xioctl(VIDIOC_REQBUFS, &req) == -1){
		_err_num = CAM_EREQBUFF;
		return false;
	}

	_buffers = (struct buffer*)calloc(req.count, sizeof(*_buffers));
	if(!_buffers){
		_err_num = 1001;
		return false;
	}

	for(n = 0; n < _n_buffers; n++){
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n;

		if(xioctl(VIDIOC_QUERYBUF, &buf) == -1){
			_err_num = CAM_EQUERYBUF;
			return false;
		}

		_buffers[n].length = buf.length;
		_buffers[n].start = (uint8_t*) mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);

		if(_buffers[n].start == MAP_FAILED){
			_err_num = CAM_EMMAP;
			return false;
		}

		memset(_buffers[n].start, 0, _buffers[n].length);
	}

	_buffer_size = _buffers[0].length;

	return true;
}

bool etm::Cam::uninit_mmap(void)
{
	uint8_t n;

	for(n = 0; n < _n_buffers; n++){
		 munmap(_buffers[n].start, _buffers[n].length);
	}

	free(_buffers);

	return true;
}

bool etm::Cam::init_dev(void)
{
	struct v4l2_capability capabilities;
	if (xioctl(VIDIOC_QUERYCAP, &capabilities) == -1){
		_err_num = CAM_EQUERYCAP;
		return false;
	}

	if(!(capabilities.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
		_err_num = CAM_ECAPCAPTURE;
		return false;
	}

	/* tulajdonságok:
	 *  	v4l2-ctl --list-formats-ext
	 *  	v4l2-ctl --list-ctrls-menus
	 */
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = _width;
	fmt.fmt.pix.height = _height;

	/* formátumok :
	 * 		http://lxr.free-electrons.com/source/include/uapi/linux/videodev2.h?v=3.14#L367
	 */
	if(strcmp(_pixel_format, "YUYV") == 0)
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	else if(strcmp(_pixel_format, "MJPG") == 0){
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	}

	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if (xioctl(VIDIOC_S_FMT, &fmt) == -1){
		_err_num = CAM_ESETFMT;
		return false;
	}

	struct v4l2_streamparm streamparm;
	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl(VIDIOC_G_PARM, &streamparm) == -1){
		_err_num = CAM_EFPS1;
		return false;
	}

	streamparm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
	streamparm.parm.capture.timeperframe.numerator = 1;
	streamparm.parm.capture.timeperframe.denominator = _fps;
	if (xioctl(VIDIOC_S_PARM, &streamparm) == -1){
		_err_num = CAM_EFPS2;
		return false;
	}

	/* beállítható paraméterek:
	 * 		http://linuxtv.org/downloads/v4l-dvb-apis/extended-controls.html#camera-controls
	 */

	struct v4l2_control cont;
	memset(&cont, 0, sizeof(cont));
	cont.id = V4L2_CID_EXPOSURE_AUTO;
	//cont.value = V4L2_EXPOSURE_MANUAL;
	cont.value = V4L2_EXPOSURE_APERTURE_PRIORITY;
	if (xioctl(VIDIOC_S_CTRL, &cont) == -1){
		_err_num = CAM_EEXPOSUREM;
		return false;
	}

	memset(&cont, 0, sizeof(cont));
	cont.id = V4L2_CID_SHARPNESS;
	//cont.value = V4L2_EXPOSURE_MANUAL;
	cont.value = 0;
	if (xioctl(VIDIOC_S_CTRL, &cont) == -1){
		_err_num = CAM_EEXPOSUREM;
		return false;
	}

	/*
	memset(&cont, 0, sizeof(cont));
	cont.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	cont.value = 625;
	if (xioctl(VIDIOC_S_CTRL, &cont) == -1){
		_err_num = CAM_EEXPOSURET;
		return false;
	}*/

	memset(&cont, 0, sizeof(cont));
	cont.id = V4L2_CID_FOCUS_AUTO;
	cont.value = 0;
	if (xioctl(VIDIOC_S_CTRL, &cont) == -1){
		_err_num = CAM_EAFOCUS;
		return false;
	}

	return true;
}

bool etm::Cam::start_stream(void)
{
	if(!_is_streaming){
		uint8_t n;

		struct v4l2_buffer buf;

		for(n = 0; n < _n_buffers; n++){
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = n;

			// Put the buffer in the incoming queue.
			if(xioctl(VIDIOC_QBUF, &buf) == -1){
				_err_num = CAM_EQBUFSS;
				return false;
			}
		}

		int type = buf.type;
		if(xioctl(VIDIOC_STREAMON, &type) == -1){
			_err_num = CAM_ESTREAMON;
			return false;
		}

		_is_streaming = true;
		return true;
	} else {
		_err_num = CAM_ESTREAMST;
		return true;
	}
}

bool etm::Cam::stop_stream(void)
{
	if(_is_streaming){
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xioctl(VIDIOC_STREAMOFF, &type) == -1) {
				_err_num = CAM_ESTREAMOFF;
				return false;
		}

		_is_streaming = false;
		return true;
	} else {
		_err_num = CAM_ESTREAMST;
		return true;
	}
}

void etm::Cam::capture_image(uint8_t *image, uint8_t mode)
{
	_is_capture_ok = true;

	if(_is_streaming){
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if(xioctl(VIDIOC_DQBUF, &buf) == -1){
			_err_num = CAM_EDQBUF1;
			_is_capture_ok = false;
			return;
		}

		if(mode == CAM_CAPTURE_FAST)
			memcpy2a(image, (uint8_t *)_buffers[buf.index].start, _buffer_size >> 1);
		else if(mode == CAM_CAPTURE_NORMAL)
			memcpy(image, (uint8_t *)_buffers[buf.index].start, _buffer_size);

		if(image == NULL){
			_err_num = CAM_ENULLIMG;
			_is_capture_ok = false;
		}

		if(xioctl(VIDIOC_QBUF, &buf) == -1){
			_err_num = CAM_EDQBUF2;
			_is_capture_ok = false;
		}
	} else {
		_err_num = CAM_ESTREAMST;
		_is_capture_ok = false;
	}
}

void etm::Cam::print_info(void)
{
	printf("Driver Caps:\n");
	struct v4l2_capability caps = {};
	if (xioctl(VIDIOC_QUERYCAP, &caps) == -1){
		printf("  Unknown\n");
		_err_num = CAM_EINFO1;
	} else {
		printf( "  Driver: \"%s\"\n"
				"  Card: \"%s\"\n"
				"  Bus: \"%s\"\n"
				"  Version: %d.%d\n"
				"  Capabilities: %08x\n",
				caps.driver,
				caps.card,
				caps.bus_info,
				(caps.version>>16)&&0xff,
				(caps.version>>24)&&0xff,
				caps.capabilities);
	}

	printf( "Camera Cropping:\n");
	struct v4l2_cropcap cropcap = {0};
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl(VIDIOC_CROPCAP, &cropcap) == -1){
			_err_num = CAM_EINFO2;
			printf("  Unknown\n");
	} else {
			printf(	"  Bounds: %dx%d\n"
					"  Default: %dx%d\n"
					"  Aspect: %d/%d\n",
					cropcap.bounds.width, cropcap.bounds.height,
					cropcap.defrect.width, cropcap.defrect.height,
					cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);
	}

	struct v4l2_fmtdesc fmtdesc = {0};
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	char fourcc[5] = {0};
	printf("Formats:\n");
	while (xioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0){
			strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
			printf("  %s (%s)\n", fourcc, fmtdesc.description);
			fmtdesc.index++;
	}

	printf("Controls:\n");
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control cont;
	struct v4l2_querymenu querymenu;
	memset (&queryctrl, 0, sizeof (queryctrl));
	for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++) {
		if (xioctl(VIDIOC_QUERYCTRL, &queryctrl) == 0) {
			  if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
			  	  continue;

			  printf(" name: %s -", queryctrl.name);
			  //printf("  id = %d", queryctrl.id);
			  printf("  min=%d", queryctrl.minimum);
			  printf("  max=%d", queryctrl.maximum);
			  printf("  step=%d", queryctrl.step);

			  memset (&cont, 0, sizeof (cont));
			  cont.id = queryctrl.id;
			  if (xioctl(VIDIOC_G_CTRL, &cont) != -1){
				  printf("  value=%d", cont.value);
			  }

			  printf("  default=%d", queryctrl.default_value);
			  switch (queryctrl.type){
				  case V4L2_CTRL_TYPE_INTEGER:
					  printf("  type=INTEGER\n");
					  break;
				  case V4L2_CTRL_TYPE_BOOLEAN:
					  printf("  type=BOOLEAN\n");
					  break;
				  case V4L2_CTRL_TYPE_MENU:
					  printf("  type=MENU\n");
					  memset (&querymenu, 0, sizeof (v4l2_querymenu));
					  querymenu.id = queryctrl.id;
					  for (querymenu.index = queryctrl.minimum; querymenu.index < (uint32_t)queryctrl.maximum + 1; querymenu.index++){
						  if (xioctl(VIDIOC_QUERYMENU, &querymenu) != -1){
							  printf("     %d. %s\n", querymenu.index, querymenu.name);
						  }
					  }
					  break;
				  case V4L2_CTRL_TYPE_BUTTON:
					  printf("  type=BUTTON\n");
					  break;
			  }
		} else {
			 if (errno == EINVAL)
				 continue;

			 perror("VIDIOC_QUERYCTRL");
			 break;
		}
	}

	printf("Extended controls:\n");
	memset (&queryctrl, 0, sizeof (queryctrl));
	for (queryctrl.id = V4L2_CID_CAMERA_CLASS_BASE; queryctrl.id < V4L2_CID_FM_TX_CLASS_BASE; queryctrl.id++) {
		if (xioctl(VIDIOC_QUERYCTRL, &queryctrl) == 0) {
			  if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				  continue;

			  printf(" name: %s -", queryctrl.name);
			  //printf("  id = %d", queryctrl.id);
			  printf("  min=%d", queryctrl.minimum);
			  printf("  max=%d", queryctrl.maximum);
			  printf("  step=%d", queryctrl.step);

			  memset (&cont, 0, sizeof (cont));
			  cont.id = queryctrl.id;
			  if (xioctl(VIDIOC_G_CTRL, &cont) != -1){
				  printf("  value=%d", cont.value);
			  }

			  printf("  default=%d", queryctrl.default_value);
			  switch (queryctrl.type){
				  case V4L2_CTRL_TYPE_INTEGER:
					  printf("  type=INTEGER\n");
					  break;
				  case V4L2_CTRL_TYPE_BOOLEAN:
					  printf("  type=BOOLEAN\n");
					  break;
				  case V4L2_CTRL_TYPE_MENU:
					  printf("  type=MENU\n");
					  memset (&querymenu, 0, sizeof (v4l2_querymenu));
					  querymenu.id = queryctrl.id;
					  for (querymenu.index = queryctrl.minimum; querymenu.index < (uint32_t)queryctrl.maximum + 1; querymenu.index++){
						  if (xioctl(VIDIOC_QUERYMENU, &querymenu) != -1){
							  printf("     %d. %s\n", querymenu.index, querymenu.name);
						  }
					  }
					  break;
				  case V4L2_CTRL_TYPE_BUTTON:
					  printf("  type=BUTTON\n");
					  break;
			  }
		} else {
			 if (errno == EINVAL)
				 continue;

			 perror("VIDIOC_QUERYCTRL");
			 break;
		}
	}
}

void etm::Cam::get_pixel_format(char *pxf) const
{
	strcpy(pxf, _pixel_format);
}

void etm::Cam::memcpy2a(uint8_t *dest, const uint8_t *src, ssize_t length)
{
		asm("mov %0, %%esi\n\t"
				: :"r"(src));  	// SRC mutató betöltése a "Source Index" registerbe

		asm("mov %0, %%edi\n\t"
				: : "r"(dest)); // DEST mutató betöltése a "Destination Index" registerbe

		asm("mov %0, %%ebx\n"  //adathossz betöltése B-be
				: : "r"(length));

		asm("copy:\n\t"
			"movb (%esi), %al\n\t"	// csak bájtokat másolunk, oda elég AL
			"movb %al, (%edi)\n\t"	// átmásolás

			"add $2, %esi\n\t"		// 2 hozzáadása a mutatóhoz -> ugrás 2 bájttal
			"add $1, %edi\n\t"		// 1 hozzáadása a mutatóhoz -> ugrás 1 bájttal
			"dec %ebx\n\t"			// számláló csökkentése

			"jnz copy\n"			// ha számláló nem nulla, ciklus megy tovább
			"copy_end:\n\t"
		);
}
