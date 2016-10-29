#ifndef __CAPREAD_H__
#define __CAPREAD_H__

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#ifndef _WIN64
#include "cap.h"
#include <inttypes.h>
#endif
#include <time.h>
#include <fcntl.h>

#ifdef _WIN64
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

typedef struct cap_header{
	uint32_t width;
	uint32_t height;
	uint32_t frame_size;
	uint32_t frame_count;
} cap_header;

#endif 


class CapRead{
	private:
	cap_header _cap_head;
	
	bool _is_open;
	int _err_num;

	FILE* _cap_fd;
	public:

	char _cap_name[256];
	double firsttime;
	double lasttime;
	double deltatime;
	
	CapRead(void);
	~CapRead(void);

	void openfile(char* cap_name);
	void getnext(uint8_t *data);
	bool islast() { return (_err_num == 7); };
	void shut(void);

	bool is_open(void) const { return _is_open; };
	int get_err_num(void) const { return _err_num; };
	
	uint32_t framesize(void) const { return _cap_head.frame_size; };
	uint32_t width(void) const { return _cap_head.width ; };
	uint32_t height(void) const { return _cap_head.height; };
	int get_frame_count(void) const { return _cap_head.frame_count; };
};


#endif
