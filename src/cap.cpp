#include "cap.h"

etm::Cap::Cap(const char *path)
{
	_is_open = false;
	_err_num = 0;

	_cap_head.width = 0;
	_cap_head.height = 0;
	_cap_head.frame_size = 0;
	_cap_head.frame_count = 0;

	strcpy(_cap_path, path);

	_alloc_size = 0;

	_del_caps();
}

etm::Cap::~Cap(void)
{
	if(_is_open)
		shut();
}

void etm::Cap::_del_caps(void){

	char cmd[512];

	sprintf(cmd, "rm -f %s/*.cap", _cap_path);

	system(cmd);
}

void etm::Cap::create(uint32_t width, uint32_t height, uint32_t frame_size, uint32_t fps, uint32_t measure_time_s)
{
	_del_caps();

	_err_num = 0;
	_is_open = false;

	_cap_head.width = width;
	_cap_head.height = height;
	_cap_head.frame_size = frame_size;
	_cap_head.frame_count = 0;

	_frame_ctr = 0;

	_alloc_size = (frame_size + sizeof(struct timespec)) * fps * measure_time_s;	// ideális esetben max ennyi képre lehet számítani

	_mem_buffer = NULL;

	_mem_buffer = (uint8_t*)malloc(_alloc_size);

	if(_mem_buffer == NULL){
		_err_num = 1;
		return;
	}

	_mem_offset = 0;

	_is_open = true;
}

void etm::Cap::append(uint8_t *data)
{
	if(!_is_open){
		_err_num = 2;
		return;
	}

	struct timespec tp;

	_err_num = 0;

	if(clock_gettime(CLOCK_REALTIME, &tp) == -1){
		_err_num = 3;
		return;
	}

	if(data == NULL){
		_err_num = 4;
		return;
	}

	memcpy(_mem_buffer + _mem_offset, data, _cap_head.frame_size);
	_mem_offset += _cap_head.frame_size;

	memcpy(_mem_buffer + _mem_offset, &tp, sizeof(tp));
	_mem_offset += sizeof(tp);

	_frame_ctr++;
}

void etm::Cap::shut(void)
{
	if(_is_open){

		uint32_t i;
		time_t t = time(NULL);          	// aktuális idõ lekérdezése
		struct tm tm = *localtime(&t);		// így olvasható formátumra hozzuk

		// fájlnév elkészítése
		sprintf(_cap_name, "%s/%04d-%02d-%02dT%02d%02d%02d.cap", _cap_path, tm.tm_year + 1900, tm.tm_mon + 1,
														   tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		_cap_fd = open(_cap_name, O_CREAT | O_WRONLY | O_TRUNC);

		if(_cap_fd == -1){
			_err_num = 5;
			return;
		}

		// header frissítjük
		_cap_head.frame_count = _frame_ctr;

		ssize_t ret_wr = write(_cap_fd, &_cap_head, sizeof(_cap_head));
		if(ret_wr != sizeof(_cap_head)){
			_err_num = 6;
			return;
		}

		_mem_offset = 0;
		for(i = 0; i < _frame_ctr; i++){
			ssize_t ret_wr = write(_cap_fd, _mem_buffer + _mem_offset, _cap_head.frame_size + sizeof(struct timespec));
			// ez hianyzott nagyon:
			_mem_offset += _cap_head.frame_size + sizeof(struct timespec);

			if(ret_wr != (ssize_t)(_cap_head.frame_size + sizeof(struct timespec))){
				_err_num = 7;
				return;
			}
		}

		free(_mem_buffer);

		fchmod(_cap_fd, S_IRUSR | S_IWUSR);		// írási és olvasási jogok a tulajdonosnak
		close(_cap_fd);							// fájl lezárása

		_is_open = false;
	}
}

void etm::Cap::del(void)
{
	if(_is_open)
		shut();

	unlink(_cap_name);		// törlés
}
