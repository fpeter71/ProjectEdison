#include "uart.h"

etm::UART::UART(int wait)
{
	_err_num = 0;
	_is_open = false;

	_uart = mraa_uart_init_raw("/dev/ttyMFD1");
	if (_uart == NULL) {
		_err_num = 1;
		return;
	}

    _error = mraa_uart_set_baudrate(_uart, 9600);
    if (_error != MRAA_SUCCESS) {
    	_err_num = 2;
		return;
    }

    _error = mraa_uart_set_mode(_uart, 8, MRAA_UART_PARITY_NONE, 1);
  	if (_error != MRAA_SUCCESS) {
  		_err_num = 3;
  		return;
  	}

  	_wait_for_read = wait;

  	usleep(100000);	// 100 ms

	_is_open = true;
}

etm::UART::~UART(void)
{
	mraa_uart_stop(_uart);
}

void etm::UART::write_ch(char ch)
{
	_err_num = 0;

	_send_buf[0] = ch;

	if(mraa_uart_write(_uart, _send_buf, 1) != 1)
		_err_num = 4;
}

void etm::UART::write_text(char *str)
{
	_err_num = 0;
	unsigned int i, ctr = 0;

	for(i = 0; str[i] != '\0'; i++){
		write_ch(str[i]);

		if(_err_num == 0){
			ctr++;
		}
	}

	if(ctr != strlen(str))
		_err_num = 5;
}

bool etm::UART::data_avaliable(void)
{
	return mraa_uart_data_available(_uart, _wait_for_read);
}

char etm::UART::read_ch(void)
{
	_err_num = 0;

	if(mraa_uart_read(_uart, _recv_buf, 1) != 1)
		_err_num = 6;

	return _recv_buf[0];
}

bool etm::UART::is_open(void) const
{
	return _is_open;
}

bool etm::UART::get_err_num(void) const
{
	return _err_num;
}
