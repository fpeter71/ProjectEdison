#ifndef __Interface_H__
#define __Interface_H__

#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <string.h>
#include <inttypes.h>

#include "info.h"
#include "comcu.h"
#include "wifi.h"

#include "Adafruit_TFTLCD.h"

namespace etm{
	class Interface : public Adafruit_TFTLCD{

private:
		char _path[256];

		double _start_time;
		double _measure_interval;
		int16_t _last_w;
		int16_t _last_w_calc;

		bool _home_screen;
		bool _rec_screen;
		bool _chip_screen;
		bool _temp_screen;
		bool _info_screen;
		bool _area_screen;
		bool _method_screen;
		bool _time_screen;
		bool _calc_screen;
		bool _res_screen;
		bool _wifi_screen;
		bool _focus_help_screen;

		cv::Mat _img_logo;
		cv::Mat _img_error;
		cv::Mat _img_home;
		cv::Mat _img_rec;
		cv::Mat _img_chip;
		cv::Mat _img_temp;
		cv::Mat _img_cow;
		cv::Mat _img_info;
		cv::Mat _img_area;
		cv::Mat _img_method;
		cv::Mat _img_time;
		cv::Mat _img_calc;
		cv::Mat _img_res;
		cv::Mat _img_wifi;
		cv::Mat _img_focus_help;

public:
		Interface(const char *path);

		void logo_screen(void);

		void focus_help_screen(void);
		void focus_help_screen_reset(void) { _focus_help_screen = false; };

		void focus_screen(uint32_t width, uint32_t height, uint8_t *data, char *pxf);
		void focus_screen(cv::Mat img);

		void home_screen(void);
		void home_screen_reset(void) { _home_screen = false; };

		void error_screen(char *fmt, ...);
		void error_screen(int32_t err_num);

		void rec_screen(double start_time, double measure_interval);
		void rec_screen_update(double time);
		void rec_screen_reset(void) { _rec_screen = false; };

		void calc_screen(void);
		void calc_screen_update(double percent);
		void calc_screen_reset(void) { _calc_screen = false; };

		void res_screen(double *results);
		void res_screen_reset(void) { _res_screen = false; };

		void temp_screen(etm::coMCU &etpic);
		void temp_screen_reset(void) { _temp_screen = false; };

		void chip_screen(void);
		void chip_screen_reset(void) { _chip_screen = false; };

		void area_screen(void);
		void area_screen_reset(void) { _area_screen = false; };

		void method_screen(void);
		void method_screen_reset(void) { _method_screen = false; };

		void time_screen(void);
		void time_screen_reset(void) { _time_screen = false; };

		void wifi_screen(char *text, bool list, etm::Wifi &etwifi);
		void wifi_screen_pos(int32_t pos);
		void wifi_screen_err(char *text, bool error);
		void wifi_screen_reset(void) { _wifi_screen = false; };

		void cow_screen(void);

		void info_screen(etm::Info &etinfo, etm::Wifi &etwifi);
		void info_screen_reset(void) { _info_screen = false; };

		bool is_rec_screen(void) { return _rec_screen; };

	};
}
#endif
