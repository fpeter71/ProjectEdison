#ifndef __CAP_H__
#define __CAP_H__

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

typedef struct cap_header{
	uint32_t width;
	uint32_t height;
	uint32_t frame_size;
	uint32_t frame_count;
}cap_header;

namespace etm {
	class Cap{
private:
	cap_header _cap_head;
	char _cap_name[256];
	char _cap_path[256];

	uint32_t _frame_ctr;
	uint32_t _alloc_size;

	uint8_t *_mem_buffer;
	uint32_t _mem_offset;

	bool _is_open;
	int _err_num;

	int32_t _cap_fd;

	void _del_caps(void);

public:
	Cap(const char *path);
	~Cap(void);

	void create(uint32_t width, uint32_t height, uint32_t frame_size, uint32_t fps, uint32_t measure_time_s);
	void append(uint8_t *data);
	void shut(void);
	void del(void);

	bool is_open(void) const { return _is_open; };
	int get_err_num(void) const { return _err_num; };

	void get_file_name(char *buf) { strcpy(buf, _cap_name); };
	};
}

#endif
