#include "info.h"

etm::Info::Info(char *dname, uint32_t v_ma, uint32_t v_mi)
{
	strcpy(_dev_name, dname);
	_ver_major = v_ma;
	_ver_minor = v_mi;
}

void etm::Info::get_kernel_ver(char *buf)
{
	FILE *in;
	char kbuf[512];
	uint8_t ctr = 0;

	if(!(in = popen("uname -r", "r"))){
		return;
	}

	while(fgets(kbuf, sizeof(kbuf), in)!= NULL){
		strcpy(buf, kbuf);
	}

	pclose(in);

	while(buf[ctr] != '-')
		ctr++;

	buf[ctr] = '\0';
}

void etm::Info::get_mem(char *buf)
{
	struct sysinfo info;
	sysinfo(&info);

	double total_ram = (double)(info.totalram * info.mem_unit);
	double free_ram = (double)(info.freeram * info.mem_unit);
	char prefixt = '\0';
	char prefixf = '\0';

	if(total_ram > 1024){
		total_ram /= 1024;
		prefixt = 'k';
	}

	if(total_ram > 1024){
		total_ram /= 1024;
		prefixt = 'M';
	}

	if(free_ram > 1024){
		free_ram /= 1024;
		prefixf = 'k';
	}

	if(free_ram > 1024){
		free_ram /= 1024;
		prefixf = 'M';
	}

	sprintf(buf, "%.2f%c / %.2f%c", free_ram, prefixf, total_ram, prefixt);
}
