#include "capread.h"

CapRead::CapRead(void)
{
	_is_open = false;
	_err_num = 0;

	_cap_head.width = 0;
	_cap_head.height = 0;
	_cap_head.frame_size = 0;
	_cap_head.frame_count = 0;
	
	firsttime = -1;
	lasttime  = 0.0;
}

CapRead::~CapRead(void)
{
	if(_is_open)
		shut();
}

void CapRead::openfile(char* cap_name)
{
	_err_num = 0;
	_is_open = false;
	strcpy(_cap_name, cap_name);

	_cap_fd = fopen(_cap_name,"rb");

	if(_cap_fd == NULL){
		_err_num = 1;
		return;
	}

	size_t ret_rd = fread(&_cap_head, 1, sizeof(_cap_head), _cap_fd);
	if(ret_rd != sizeof(_cap_head)){
		_err_num = 2;
		return;
	}

	_is_open = true;
}

void CapRead::getnext(uint8_t *data)
{
	_err_num = 0;

	if(!_is_open){
		_err_num = 3;
		return;
	}
	
	if(data == NULL){
		_err_num = 5;
		return;
	}

	size_t ret_rd = fread(data, 1, _cap_head.frame_size, _cap_fd);
	if(ret_rd != (size_t)_cap_head.frame_size){
		_err_num = 7;
		return;
	}

	uint32_t timestamp[2];
	ret_rd = fread(timestamp, 1, 2*sizeof(uint32_t), _cap_fd);
	if(ret_rd != 2*(size_t)(sizeof(uint32_t))){
		_err_num = 8;
		return;
	}
	// timestamp = minute*1e3 + nsec / 1e6 - timestampbase;
    // timestampbase = minute*1e3 + nsec / 1e6;
	if (firsttime == -1) {
		firsttime = timestamp[0] + timestamp[1]/1e9;
	}

	double currtime = timestamp[0] + timestamp[1]/1e9 - firsttime;
	
	deltatime = currtime - lasttime;
	lasttime = currtime;
}

void CapRead::shut(void)
{
	if(_is_open){
		fclose(_cap_fd);

		_is_open = false;
	}
}
