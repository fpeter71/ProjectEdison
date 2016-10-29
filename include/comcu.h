#ifndef __COMCU_H__
#define __COMCU_H__

#include <time.h>

#include "uart.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

#define	CMD_GET_TEMP		"#T;"
#define CMD_MOT_PLUS(X)		"#+"X";"
#define CMD_MOT_MINUS(X)	"#-"X";"
#define CMD_MOT_OFF			"#*;"
#define CMD_HEATING_HEAT	"#PH;"
#define CMD_HEATING_COOL	"#PC;"
#define CMD_HEATING_OFF		"#PF;"
#define CMD_LED_ON			"#LN;"
#define CMD_LED_OFF			"#LF;"
#define CMD_COM_CHECK		"#K;"
#define CMD_CHIP_CHECK		"#C;"
#define CMD_BUTTONS_STATE	"#B;"
#define CMD_BATT_CHECK		"#A;"

#define BTN_A_MASK	8
#define BTN_B_MASK	4
#define BTN_C_MASK	2
#define BTN_D_MASK	1

#define COM_OK		'K'

#define	CHIP_OK		'Y'
#define	CHIP_NOK	'N'

#define	BATT_OK		'F'
#define BATT_CHARG	'E'
#define BATT_DEAD	'G'

#define	TEMP_RESOLUTION 	9
#define RES_SHIFT  			(TEMP_RESOLUTION - 8)

namespace etm{
	class coMCU : public UART{

public:
	coMCU(float tmin, float tmax);

	void stop(void);

	void send_cmd(char *cmd);

	uint8_t get_buttons_state(void);

	float get_temp(void);
	void write_temp(void);

	void cam_led_on(void);
	void cam_led_off(void);

	void start_temp_control(void);
	void stop_temp_control(void);
	bool is_temp_ok(void);

	bool is_chip_in(void);

private:
	bool _com_in_progress;
	float _temp_min, _temp_max;

	float convert_temp(unsigned int temp);

	};
}

#endif
