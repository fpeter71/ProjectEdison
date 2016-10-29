#ifndef __UART_H__
#define __UART_H__

#include "mraa.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace etm{

	class UART {

protected:
			mraa_result_t _error;
			mraa_uart_context _uart;

			char _send_buf[16];
			char _recv_buf[16];

			int _wait_for_read;

			int _err_num;
			bool _is_open;

public:
			UART(int wait);
			~UART(void);

			bool data_avaliable(void);

			void write_ch(char ch);
			void write_text(char *str);

			char read_ch(void);

			bool is_open(void) const;
			bool get_err_num(void) const;
	};

}


#endif
