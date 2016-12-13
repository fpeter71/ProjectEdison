#include "comcu.h"

etm::coMCU::coMCU(float tmin, float tmax) : UART(500)
{
	_com_in_progress = false;
	_temp_min = tmin;
	_temp_max = tmax;
}

void etm::coMCU::stop(void)
{
	stop_temp_control();
	cam_led_off();
}

void etm::coMCU::send_cmd(char *cmd)
{
	write_text(cmd);
}

uint8_t etm::coMCU::get_buttons_state(void)
{
	uint8_t ret = 0;

	send_cmd(CMD_BUTTONS_STATE);

	if(data_avaliable())
		ret = read_ch();

	return ret;
}

float etm::coMCU::get_temp(void)
{
	/*
	_err_num = 0;

	char T1, T2;
	unsigned int temp = 0x00;

	send_cmd(CMD_GET_TEMP);

	if(data_avaliable()){
		T1 = read_ch();
		T2 = read_ch();

		if(_err_num == 0){
			temp = (T2 << 8) + T1;
		}
	} else {
		_err_num = 10;
	}

	return convert_temp(temp);
	*/

	char text[15];
	memset(text, 0, 15);
	get_temp_str(text);

	return atof(text);
}

void etm::coMCU::get_temp_str(char *s)
{
	send_cmd(CMD_GET_TEMP_STR);
	int i = 0;
	char ch;

	while(data_avaliable()){
		 ch = read_ch();
	 	 if(ch == '\n' || ch == '\r')
	 		 break;

	 	 s[i++] = ch;
	}

	while(data_avaliable())
		ch = read_ch();

}

float etm::coMCU::convert_temp(unsigned int temp)
{
	unsigned int temp2write = temp;
	char tempWhole = 0;

	// Extract tempWhole
	tempWhole = temp2write >> RES_SHIFT ;

	return tempWhole;
}

void etm::coMCU::write_temp(void)
{
	float temp = get_temp();
	printf("Temp: %f oC\n", temp);
}

void etm::coMCU::start_temp_control(void)
{
	send_cmd(CMD_HEATING_HEAT);
}

void etm::coMCU::stop_temp_control(void)
{
	uint8_t i;

	for(i = 0; i < 5; i++){
		send_cmd(CMD_HEATING_OFF);
		usleep(1000);
	}
}

bool etm::coMCU::is_temp_ok(void)
{
	float temp = get_temp();

	if(temp >= _temp_min && temp <= _temp_max)
		return true;
	else
		return false;
}

bool etm::coMCU::is_chip_in(void)
{
	uint8_t ch = CHIP_NOK;

	send_cmd(CMD_CHIP_CHECK);

	if(data_avaliable())
		ch = read_ch();

	if(ch == CHIP_OK)
		return true;
	else
		return false;
}

void etm::coMCU::cam_led_on(void)
{
	uint8_t i;

	for(i = 0; i < 5; i++){
		send_cmd(CMD_LED_ON);
		usleep(1000);
	}
}

void etm::coMCU::cam_led_off(void)
{
	uint8_t i;

	for(i = 0; i < 5; i++){
		send_cmd(CMD_LED_OFF);
		usleep(1000);
	}
}
