#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <ctype.h>

#define WIFI_ENTRIES_SIZE	30

typedef struct wifi_entry{
	char quality[8];
	char essid[64];
}wifi_entry;

namespace etm {
	class Wifi{

public:
		Wifi(char *dev);

		bool is_enabled(void);

		void enable(void);
		void disable(void);

		void scan(void);

		void connect(char *ESSID);
		void connect(uint32_t idx);

		void get_wifi_name(uint32_t idx, char *buf);

		uint32_t get_wifi_ctr(void) { return _wifi_ctr; };

		int get_err_num(void) { return _err_num; };

		void get_ip(char *buf);

		bool is_connected(char *ESSID);

private:
		wifi_entry _entries[WIFI_ENTRIES_SIZE];
		uint32_t _wifi_ctr;

		char _device[16];
		char _network[16];
		bool _enabled;

		void get_value(char *src, char term, char *value);
		void copy_wo_quotes(char *src, char *dest);
		void copy_wo_specs(char *src, char *dest);

		FILE *_pipe;
		FILE *_fp;

		int _err_num;
	};
}
#endif
