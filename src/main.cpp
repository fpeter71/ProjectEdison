#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include "opencv2/opencv.hpp"

#include "log.h"
#include "cam.h"
#include "interface.h"
#include "comcu.h"
#include "cap.h"
#include "info.h"

#include "SpermDetectors.h"
#include <opencv2/opencv.hpp>

// megszabadulunk a "deprecated conversion from string constant to 'char*' [-Wwrite-strings]" figyelmeztetést?l
#pragma GCC diagnostic ignored "-Wwrite-strings"

enum program_state { home, method, area, timeofrec, heatandchip, focus, recandcalc, res, wifi, info };

double get_millis(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);

	return ((tp.tv_sec) * 1000.0 + (tp.tv_nsec) / 1.0e6);
}

void mssleep (uint32_t ms)
{
	usleep(ms * 1000);
}

uint8_t *img_buffer_fast = NULL;
uint8_t *img_buffer_normal = NULL;
ssize_t img_buffer_size_fast = 0;
ssize_t img_buffer_size_normal = 0;
char pixel_format[5];

uint32_t run_time;
double start_time, end_time;
double measure_start_time = 0;
uint8_t measure_first = 1;

bool running = true;
program_state ps = home;
int return_number = 0;
uint8_t buttons_state = 0;

int detection_method = -1;
int area_of_pic = 1;
double measure_time = 10.0 * 1000.0; // msec
double results[6];

#define INI_PATH	"/etc/bgt/MFL_settings.ini"

void sig_handler(int signo)
{
	// Ctrl + C
	if (signo == SIGINT) {
		running = false;
	}
}

void *thread_fcn(void *threadarg);
void focus_sub(etm::Log &etlog, etm::Cam &etcam, etm::coMCU &etpic, etm::Interface &etinterf);
void capture_sub(etm::Log &etlog, etm::Cam &etcam, etm::coMCU &etpic, etm::Interface &etinterf, etm::Cap &etcap);
void temp_sub(etm::coMCU &etpic);

int main(int argc, char **argv)
{
	int a;
	bool silent = false;

	for(a = 0; a < argc; a++){
		if(argv[a][0] == '-'){
			switch(argv[a][1]){
				case 's' :
					silent = true;
					break;
			}
		}
	}

	signal(SIGINT, sig_handler);

	etm::Log etlog("/var/log/bgt.log", LOG_ERROR | LOG_WARNING | LOG_INFO | LOG_DISPLAY, !silent);
	etm::Cam etcam("/dev/video0", 800, 600, "YUYV", 24);
	etm::coMCU etpic(30.0, 37.0);
	etm::Interface etinterf("/opt/bgt/pix");
	etm::Cap etcap("/home/root");
	etm::Info etinfo("MFL202M", 1, 0);
	etm::Wifi etwifi("wlan0");

    if(!etlog.is_open()){
    	if(!silent){
    		fprintf(stderr, "unable to create log (%d)\n", etlog.get_err_num());
    	}
    	return_number = 1;
    	goto prog_end;
    }
    etlog.write(LOG_INFO, "log initialized");

	if(!etinterf.is_open()){
		etlog.writef(LOG_ERROR, "unable to initialize the interface (%d)", etinterf.get_err_num());
		return_number = 2;
		goto prog_end;
	}

	etinterf.reset();
	etinterf.begin();

	etinterf.logo_screen();
	etlog.write(LOG_INFO, "interface initialized");

	if(!etpic.is_open()){
		etlog.writef(LOG_ERROR, "unable to initialize the co-MCU (%d)", etpic.get_err_num());
		etinterf.error_screen(3);
		return_number = 3;
		goto displ_end;
	}

	etpic.send_cmd(CMD_COM_CHECK);
	if(etpic.data_avaliable()){
		uint8_t ch = etpic.read_ch();
		if(ch != COM_OK){
			etlog.writef(LOG_ERROR, "unable to communicate with the co-MCU(bad data : %d (%c))", ch, ch);
			etinterf.error_screen(4);
			return_number = 4;
			goto displ_end;
		}
	} else {
		etlog.writef(LOG_ERROR, "unable to communicate with the co-MCU(no data)");
		etinterf.error_screen(5);
		return_number = 5;
		goto displ_end;
	}
	etlog.write(LOG_INFO, "co-MCU initialized");

    if(!etcam.is_open()){
    	etlog.writef(LOG_ERROR, "unable to initialize the camera (%d)", etcam.get_err_num());
    	etinterf.error_screen(7);
    	return_number = 7;
    	goto displ_end;
    }
    img_buffer_size_normal = etcam.get_buffer_size();
    img_buffer_size_fast = img_buffer_size_normal >> 1;	// 2
    img_buffer_normal = (uint8_t*)malloc(img_buffer_size_normal);
    img_buffer_fast = (uint8_t*)malloc(img_buffer_size_fast);
    etcam.get_pixel_format(pixel_format);
    run_time = (uint32_t) ceil(1000.0 / (etcam.get_fps() * 1.0));

    etlog.write(LOG_INFO, "camera initialized");
    //etcam.print_info();

    mssleep(2000);
    while(running){
    	buttons_state = etpic.get_buttons_state();
    	if(ps == home){
        	etinterf.home_screen();

    		if(buttons_state & BTN_A_MASK){
    			ps = heatandchip;

    			etinterf.home_screen_reset();
    			etpic.start_temp_control();

				while(!etpic.is_temp_ok()){
					etinterf.temp_screen(etpic);
					mssleep(250);

					// csak h ki lehessen lépni
					if(!running)
						goto cam_end;
				}
				etinterf.temp_screen_reset();

				for(;;){

					buttons_state = etpic.get_buttons_state();
					etinterf.chip_screen();

					if(buttons_state & BTN_C_MASK){
						if(etpic.is_chip_in()){
							break;
						}
					}else if(buttons_state & BTN_D_MASK){
						etinterf.chip_screen_reset();
						ps = home;
						break;
					}

					if(!running){
						return_number = 10;
						goto cam_end;
					}

					mssleep(250);
				}

    			etinterf.chip_screen_reset();

    			ps = method;

    		} else if(buttons_state & BTN_B_MASK){
    			ps = wifi;
    		} else if(buttons_state & BTN_C_MASK){
    			ps = info;
    		}

    	} else if(ps == method) {
    		etinterf.method_screen();

    		if(buttons_state & BTN_A_MASK){
    			detection_method = 0;
    			ps = area;
    			etinterf.method_screen_reset();
			} else if(buttons_state & BTN_B_MASK){
				detection_method = 1;
				ps = area;
				etinterf.method_screen_reset();
			} else if(buttons_state & BTN_C_MASK){
				detection_method = -1;
				ps = area;
				etinterf.method_screen_reset();
			} else if(buttons_state & BTN_D_MASK){
				ps = home;
				etinterf.method_screen_reset();
			}

    	} else if(ps == area) {
    		etinterf.area_screen();

    		if(buttons_state & BTN_A_MASK){
    			area_of_pic = 1;
    			ps = timeofrec;
    			etinterf.area_screen_reset();
			} else if(buttons_state & BTN_B_MASK){
				area_of_pic = 2;
				ps = timeofrec;
				etinterf.area_screen_reset();
			} else if(buttons_state & BTN_C_MASK){
				area_of_pic = 4;
				ps = timeofrec;
				etinterf.area_screen_reset();
			} else if(buttons_state & BTN_D_MASK){
				ps = method;
				etinterf.area_screen_reset();
			}

    	} else if(ps == timeofrec) {
			etinterf.time_screen();

			if(buttons_state & BTN_A_MASK){
				measure_time = 5 * 1000;
				ps = focus;
				etinterf.time_screen_reset();
			} else if(buttons_state & BTN_B_MASK){
				measure_time = 10 * 1000;
				ps = focus;
				etinterf.time_screen_reset();
			} else if(buttons_state & BTN_C_MASK){
				measure_time = 30 * 1000;
				ps = focus;
				etinterf.time_screen_reset();
			} else if(buttons_state & BTN_D_MASK){
				ps = area;
				etinterf.time_screen_reset();
			}

		} else if(ps == focus){

			etinterf.focus_help_screen();

			for(;;){

				buttons_state = etpic.get_buttons_state();

				if(buttons_state & BTN_C_MASK){
					break;
				}else if(buttons_state & BTN_D_MASK){
					etinterf.focus_help_screen_reset();
					ps = timeofrec;
					break;
				}

				if(!running){
					return_number = 10;
					goto cam_end;
				}

				mssleep(250);
			}

			etinterf.focus_help_screen_reset();

    		focus_sub(etlog, etcam, etpic, etinterf);

			if(return_number != 0){
				goto cam_end;
			}

    	} else if(ps == recandcalc){
    		measure_first = 1;
			capture_sub(etlog, etcam, etpic, etinterf, etcap);

			// nem indult el a stream
			if(return_number != 0){
				goto cam_end;
			}

			char fname[256];

			etcap.get_file_name(fname);

			etinterf.calc_screen();

			AnalyseVideo(fname, etinterf, INI_PATH, detection_method, area_of_pic, (int)(measure_time / 1000.0), "", "", "", results);

			etinterf.calc_screen_reset();

			etpic.stop_temp_control();

			// atment a res ablak vegere, hogy legyen lehetoseg letolteni torles elott
			//etcap.del();

			ps = res;

    	} else if(ps == res){
    		etinterf.res_screen(results);

    		if(buttons_state & BTN_D_MASK){
				
				// torles csak itt
				etcap.del();

    			etinterf.res_screen_reset();

    			ps = home;
    		}
    	}else if(ps == wifi){

    		etwifi.enable();

    		etinterf.wifi_screen("Scanning...", false, etwifi);

    		etwifi.scan();

    		etinterf.wifi_screen("Disable Wi-Fi", true, etwifi);

    		int32_t pos = 0;

    		for(;;){

    			buttons_state = etpic.get_buttons_state();

    			etinterf.wifi_screen_pos(pos);

				if(buttons_state & BTN_D_MASK){
					etinterf.wifi_screen_reset();
					etinterf.home_screen_reset();

					ps = home;
					break;
				} else if(buttons_state & BTN_B_MASK){
					pos++;

					if(pos >= (int32_t) etwifi.get_wifi_ctr())
						pos = (int32_t) etwifi.get_wifi_ctr();

				} else if(buttons_state & BTN_A_MASK){
					pos--;

					if(pos < 0)
						pos = 0;
				}else if(buttons_state & BTN_C_MASK){

					if(pos == 0){
						etwifi.disable();

						etinterf.wifi_screen_err("Device disabled", false);

					} else {
						etwifi.connect(pos - 1);

						if(etwifi.get_err_num() == 0){
							char ip[16], buf[128];

							etwifi.get_ip(ip);

							sprintf(buf, "Connected\n IP:%s", ip);

							etinterf.wifi_screen_err(buf, false);
						} else {
							etinterf.wifi_screen_err("Unable to connect", true);
						}
					}
				}

				if(!running){
					return_number = 10;
					break;
				}

				mssleep(250);
    		}

    	} else if(ps == info){
    		etinterf.info_screen(etinfo, etwifi);

    		if(buttons_state & BTN_D_MASK){
    			etinterf.info_screen_reset();
    			etinterf.home_screen_reset();

      			ps = home;
    		}
    	}
    	mssleep(250);
    }

cam_end:
	free(img_buffer_normal);
	free(img_buffer_fast);

	etpic.stop();

displ_end:
	if(return_number == 0)
		etinterf.cow_screen();
	else
		etinterf.error_screen(return_number);

	etinterf.end();
	etlog.write(LOG_INFO, "display stopped");

prog_end:
    exit(return_number);
}

void focus_sub(etm::Log &etlog, etm::Cam &etcam, etm::coMCU &etpic, etm::Interface &etinterf)
{
	if(etcam.start_stream()){
		etlog.write(LOG_INFO, "stream started");
	} else {
		etlog.writef(LOG_ERROR, "unable to start the stream (%d)", etcam.get_err_num());
		etinterf.error_screen(8);
		return_number = 8;
		return;
	}

	etpic.cam_led_on();

	for(;;){

		buttons_state = etpic.get_buttons_state();

		for(;;){
			etcam.capture_image(img_buffer_fast, CAM_CAPTURE_FAST);

			if(etcam.is_capture_ok())
				break;

			mssleep(1);
		}

		if(etcam.is_capture_ok()){
			cv::Mat yuyv(etcam.get_height(), etcam.get_width(), CV_8UC1, img_buffer_fast);
			cv::Mat sperm = ColorSperms(detection_method, yuyv, 240, 320, INI_PATH);

			etinterf.focus_screen(sperm);
		} else {
			etlog.writef(LOG_ERROR, "unable to capture the frame (%d)", etcam.get_err_num());
		}

		if(!running){
			return_number = 10;
			break;
		}

		if(buttons_state & BTN_A_MASK){
			etpic.send_cmd(CMD_MOT_PLUS("5"));
		} else if(buttons_state & BTN_B_MASK){
			etpic.send_cmd(CMD_MOT_MINUS("5"));
		} else if(buttons_state & BTN_C_MASK){
			ps = recandcalc;
			break;
		} else if(buttons_state & BTN_D_MASK){
			ps = timeofrec;

			etpic.stop();

			if(etcam.stop_stream()){
				etlog.write(LOG_INFO, "stream stopped");
			} else {
				etlog.writef(LOG_ERROR, "unable to stop the stream (%d)", etcam.get_err_num());
			}

			break;
		}
	}
}

void capture_sub(etm::Log &etlog, etm::Cam &etcam, etm::coMCU &etpic, etm::Interface &etinterf, etm::Cap &etcap)
{
	etcap.create(etcam.get_width(), etcam.get_height(), img_buffer_size_fast, etcam.get_fps(), (uint32_t)(measure_time / 1000.0));
	if(etcap.get_err_num() != 0){
		etlog.writef(LOG_ERROR, "unable to create cap buffer (%d)", etcap.get_err_num());
		etinterf.error_screen(9);
		return_number = 9;
		return;
	}

	for(;;){

		start_time = get_millis();

		if(measure_first){
			measure_start_time = start_time;
			measure_first = 0;
		}

		if(!measure_first && !etinterf.is_rec_screen()){
			etinterf.rec_screen(measure_start_time, measure_time);
		}

		for(;;){
			etcam.capture_image(img_buffer_fast, CAM_CAPTURE_FAST);

			if(etcam.is_capture_ok())
				break;

			mssleep(1);
		}

		if(etcam.is_capture_ok()){
			etcap.append(img_buffer_fast);

			if(etcap.get_err_num() != 0){
				etlog.writef(LOG_ERROR, "unable to write to the cap buffer (%d)", etcap.get_err_num());
			}

		} else {
			etlog.writef(LOG_ERROR, "unable to capture the frame (%d)", etcam.get_err_num());
		}

		end_time = get_millis();

		etinterf.rec_screen_update(end_time);

		if(end_time - start_time < (double)run_time){
			usleep((uint32_t)((run_time - (end_time - start_time))) * 1000);
		}

		//printf("Time: %f ms\n", get_millis() - start_time);

		if(end_time - measure_start_time >= measure_time)
			break;

		if(!running){
			return_number = 10;
			break;
		}
	}

	etpic.cam_led_off();

	etcap.shut();

	if(etcap.get_err_num() != 0){
		etlog.writef(LOG_ERROR, "unable to write to the cap file (%d)", etcap.get_err_num());
	}

	etinterf.rec_screen_reset();

	if(etcam.stop_stream()){
		etlog.write(LOG_INFO, "stream stopped");
	} else {
		etlog.writef(LOG_ERROR, "unable to stop the stream (%d)", etcam.get_err_num());
	}

	return_number = 0;
}
