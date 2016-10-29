#ifndef __INFO_H__
#define __INFO_H__

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/sysinfo.h>

namespace etm {
	class Info{

public:
	Info(char *dname, uint32_t v_ma, uint32_t v_mi);

	void get_dev_name(char *buf) { strcpy(buf, _dev_name); };
	uint32_t get_ver_major(void) { return _ver_major; };
	uint32_t get_ver_minor(void) { return _ver_minor; };

	void get_kernel_ver(char *buf);
	void get_mem(char *buf);

private:
	char _dev_name[256];
	uint32_t _ver_major;
	uint32_t _ver_minor;
	};
}

#endif
