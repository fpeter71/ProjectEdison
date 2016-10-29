#include "interface.h"

etm::Interface::Interface(const char *path) : Adafruit_TFTLCD()
{
	_is_open = false;

	strcpy(_path, path);

	char buffer[512];

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/logo.jpg", _path);
	_img_logo = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);;

	if(!_img_logo.data){
		_err_num = 1;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/error.jpg", _path);
	_img_error = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_error.data){
		_err_num = 2;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/home.jpg", _path);
	_img_home = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_home.data){
		_err_num = 3;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/recording.jpg", _path);
	_img_rec = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_rec.data){
		_err_num = 4;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/samplein.jpg", _path);
	_img_chip = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_chip.data){
		_err_num = 5;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/heatingup.jpg", _path);
	_img_temp = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_temp.data){
		_err_num = 6;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/cow.jpg", _path);
	_img_cow = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_cow.data){
		_err_num = 7;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/results.jpg", _path);
	_img_info = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_info.data){
		_err_num = 8;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/areaselection.jpg", _path);
	_img_area = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_area.data){
		_err_num = 9;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/methodselection.jpg", _path);
	_img_method = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_method.data){
		_err_num = 10;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/timeofrecording.jpg", _path);
	_img_time = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_time.data){
		_err_num = 11;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/calculating.jpg", _path);
	_img_calc = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_calc.data){
		_err_num = 12;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/wifi.jpg", _path);
	_img_wifi = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_wifi.data){
		_err_num = 13;
		return;
	}

	memset(buffer, 0, 512);
	sprintf(buffer, "%s/focushelp.png", _path);
	_img_focus_help = cv::imread(buffer, CV_LOAD_IMAGE_COLOR);

	if(!_img_focus_help.data){
		_err_num = 14;
		return;
	}

	_img_res = _img_info;

	_home_screen = false;
	_rec_screen = false;
	_chip_screen = false;
	_temp_screen = false;
	_info_screen = false;
	_area_screen = false;
	_method_screen = false;
	_time_screen = false;
	_calc_screen = false;
	_res_screen = false;
	_wifi_screen = false;
	_focus_help_screen = false;

	_is_open = true;
}

void etm::Interface::logo_screen(void)
{
	_img = _img_logo;

	set_text_color(ILI9341_WHITE);
	set_text_size(2);
	set_cursor(140, 290);

	fill_screen(ILI9341_BLACK);
	draw_image(0, 0);
	// println("MFL202M"); rossz helyen volt, nem vacakoltam vele.
}

void etm::Interface::error_screen(char *fmt, ...)
{
	_img = _img_error;

	char msg[100];
	va_list args;
	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);

	set_text_color(ILI9341_RED);
	set_text_size(2);
	set_cursor(0, 220);

	fill_screen(ILI9341_BLACK);
	draw_image(0, 0);
	println(msg);
}

void etm::Interface::error_screen(int32_t err)
{
	_img = _img_error;

	char msg[100];
	sprintf(msg, "%d", err);
	set_text_color(ILI9341_RED);
	set_text_size(3);
	set_cursor(110, 230);

	fill_screen(ILI9341_BLACK);
	draw_image(0, 0);
	println(msg);
}

void etm::Interface::home_screen(void)
{
	if(_home_screen)
		return;

	_img = _img_home;

	draw_image(0, 0);

	_home_screen = true;
}

void etm::Interface::focus_help_screen(void)
{
	if(_focus_help_screen)
		return;

	_img = _img_focus_help;

	draw_image(0, 0);

	_focus_help_screen = true;
}

void etm::Interface::focus_screen(uint32_t width, uint32_t height, uint8_t *data, char *pxf)
{
	cv::Mat frame(height, width, CV_8UC3);

	if(strcmp(pxf, "YUYV") == 0){
		cv::Mat yuyv(height, width, CV_8UC2, data);
		cv::cvtColor(yuyv, frame, CV_YUV2BGR_YUYV);
	}

	set_image(frame);
	resize_image(320, 240);
	draw_image(0, 0);
}

void etm::Interface::focus_screen(cv::Mat img)
{
	set_image(img);
	draw_image(0, 0);
}

void etm::Interface::rec_screen(double start_time, double measure_interval)
{
	_img = _img_rec;
	_start_time = start_time;
	_measure_interval = measure_interval;

	set_text_color(ILI9341_WHITE);

	draw_image(0, 0);


	fill_rect(10, 250, 230, 20, ILI9341_WHITE);

	_last_w = 0;

	_rec_screen = true;
}

void etm::Interface::rec_screen_update(double time)
{
	if(!_rec_screen)
		return;

	double tdiff = time - _start_time;
	double percent = tdiff / _measure_interval;

	// Ă­gy kevesebb adatot kell kikĂĽldeni
	int16_t w = (int16_t)(230 * percent);
	fill_rect(10 + _last_w, 250, w - _last_w, 20, ILI9341_GREEN);
	_last_w = w;
}

void etm::Interface::temp_screen(etm::coMCU &etpic)
{
	char buf[32];

	set_text_color(ILI9341_WHITE);
	set_text_size(3);

	if(!_temp_screen)
	{
		_img = _img_temp;

		draw_image(0, 0);
	}

	int32_t t = (int32_t)etpic.get_temp();

	if(t != 0){
		sprintf(buf, "%d oC", t);
		set_cursor(30, 260);
		fill_rect(0, 260, 240, 20, ILI9341_BLACK);
		println(buf);
	}

	_temp_screen = true;
}

void etm::Interface::chip_screen(void)
{
	if(_chip_screen)
		return;

	_img = _img_chip;

	set_text_color(ILI9341_WHITE);
	set_text_size(3);

	draw_image(0, 0);

	_chip_screen = true;
}

void etm::Interface::cow_screen(void)
{
	_img = _img_cow;
	draw_image(0, 0);
}

void etm::Interface::info_screen(etm::Info &etinfo, etm::Wifi &etwifi)
{
	char buf1[256], buf2[256];
	set_text_color(ILI9341_WHITE);
	set_text_size(2);

	if(!_info_screen){
		_img = _img_info;
		draw_image(0, 0);

		etinfo.get_dev_name(buf1);
		sprintf(buf2, "Device: %s", buf1);
		set_cursor(10, 50);
		println(buf2);

		sprintf(buf2, "Software: v%d.%d", etinfo.get_ver_major(), etinfo.get_ver_minor());
		set_cursor(10, 70);
		println(buf2);

		etinfo.get_kernel_ver(buf1);
		sprintf(buf2, "Kernel: %s", buf1);
		set_cursor(10, 90);
		println(buf2);
	}

	etinfo.get_mem(buf1);
	sprintf(buf2, "Mem (free/total):\n %s", buf1);
	fill_rect(0, 120, 240, 20, ILI9341_BLACK);
	set_cursor(10, 110);
	println(buf2);

	char ip[16], essid[64];

	set_cursor(10, 150);
	if(etwifi.is_connected(essid)){
		etwifi.get_ip(ip);
		sprintf(buf1, "Wi-Fi: Connected\n %s@%s", essid, ip);

		println(buf1);
	} else {
		println("Wi-Fi: Not connected");
	}

	_info_screen = true;
}


void etm::Interface::area_screen(void)
{
	if(_area_screen)
		return;

	_img = _img_area;
	draw_image(0, 0);

	_area_screen = true;
}

void etm::Interface::method_screen(void)
{
	if(_method_screen)
		return;

	_img = _img_method;
	draw_image(0, 0);

	_method_screen = true;
}

void etm::Interface::time_screen(void)
{
	if(_time_screen)
		return;

	_img = _img_time;
	draw_image(0, 0);

	_time_screen = true;
}

void etm::Interface::calc_screen(void)
{
	if(_calc_screen)
		return;

	_img = _img_calc;
	draw_image(0, 0);

	fill_rect(10, 250, 230, 20, ILI9341_WHITE);

	_last_w_calc = 0;

	_calc_screen = true;
}

void etm::Interface::calc_screen_update(double percent)
{
	int16_t w = (int16_t)(230 * percent);
	printf("-----%f\n", percent);
	fill_rect(10 + _last_w_calc, 250, w - _last_w_calc, 20, ILI9341_GREEN);
	_last_w_calc = w;
}

void etm::Interface::res_screen(double *results)
{
	if(_res_screen)
		return;

	char buf[32];

	set_text_color(ILI9341_WHITE);
	set_text_size(2);

	_img = _img_res;
	draw_image(0, 0);

	sprintf(buf, "WHO A grade: %.1lf %%", results[1]);
	set_cursor(0, 55);
	println(buf);

	sprintf(buf, "WHO B grade: %.1lf %%", results[2]);
	set_cursor(0, 85);
	println(buf);

	sprintf(buf, "WHO C grade: %.1lf %%", results[3]);
	set_cursor(0, 115);
	println(buf);

	sprintf(buf, "WHO D grade: %.1lf %%", results[4]);
	set_cursor(0, 145);
	println(buf);

	sprintf(buf, "Sperm count: %d", (int)results[5]);
	set_cursor(0, 195);
	println(buf);
	
	set_cursor(0, 210);
	println("Concentration:");

	sprintf(buf, "%.1lf millions/ml", results[0]);
	set_cursor(0, 225);
	println(buf);

	_res_screen = true;
}

void etm::Interface::wifi_screen(char *text, bool list, etm::Wifi &etwifi)
{
	uint32_t i;
	char essid[64];

	set_text_color(ILI9341_WHITE);
	set_text_size(2);

	if(list)
		set_cursor(10, 0);
	else
		set_cursor(10, 10);

	_img = _img_wifi;
	draw_image(0, 0);

	println(text);

	if(list){
		for(i = 0; i < etwifi.get_wifi_ctr(); i++){
			etwifi.get_wifi_name(i, essid);
			set_cursor(10, 18 + i * 15);
			println(essid);
		}
	}

	if(list){
		set_cursor(0, 0);
		write('>');
	}
}

void etm::Interface::wifi_screen_pos(int32_t pos)
{
	set_text_color(ILI9341_WHITE);
	fill_rect(0, 0, 10, 320, ILI9341_BLACK);
	set_cursor(0, pos * 15);
	write('>');
}

void etm::Interface::wifi_screen_err(char *text, bool error)
{
	set_text_size(2);
	set_cursor(10, 240);

	if(error){
		set_text_color(ILI9341_RED);
		println(text);
	} else {
		set_text_color(ILI9341_GREEN);
		println(text);
	}
}
