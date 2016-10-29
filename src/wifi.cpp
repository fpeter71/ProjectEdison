#include "wifi.h"

etm::Wifi::Wifi(char *dev)
{
	strcpy(_device, dev);

	_enabled = is_enabled();
}

bool etm::Wifi::is_enabled(void)
{
	char line[256];
	bool found = false;

	if((_pipe = popen("systemctl status wpa_supplicant | grep \"active (running)\" | wc -l", "r")) == NULL){
		return false;
	}

	if(fgets(line, sizeof(line), _pipe) != NULL){

		int active = atoi(line);

		if(active == 1)
			found = true;
	}

	pclose(_pipe);

	return found;
}

void etm::Wifi::enable(void)
{
	if(!_enabled){
		char cmd[256];
		sprintf(cmd, "ifconfig %s up", _device);
		system(cmd);

		usleep(10000); 		// 10 ms

		_enabled = true;
	}
}

void etm::Wifi::disable(void)
{
	char cmd[256], line[256];

	if(_enabled){
		_err_num = 0;

		if(isdigit(_network[0])){

			sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s remove_network %s", _device, _network);

			if((_pipe = popen(cmd, "r")) == NULL){
					_err_num = 20;
					return;
			}

			while(fgets(line, sizeof(line), _pipe) != NULL){

				if(strncmp(line, "OK", 2) == 0)
					break;

				if(strncmp(line, "FAIL", 4) == 0){
					_err_num = 21;
					return;
				}
			}
			pclose(_pipe);
		}

		sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s disconnect", _device);
		if((_pipe = popen(cmd, "r")) == NULL){
				_err_num = 22;
				return;
		}

		while(fgets(line, sizeof(line), _pipe) != NULL){
			if(strncmp(line, "OK", 2) == 0)
				break;

			if(strncmp(line, "FAIL", 4) == 0){
				_err_num = 23;
				break;
			}
		}

		pclose(_pipe);

		usleep(100000);

		sprintf(cmd, "ifconfig %s down", _device);
		system(cmd);

		_enabled = false;
	}
}

bool etm::Wifi::is_connected(char *ESSID)
{
	char line[256], id[128], cmd[128];
	bool found = false;

	sprintf(cmd, "iwconfig %s | grep ESSID", _device);

	if(!_enabled)
		return false;

	if((_pipe = popen(cmd, "r")) == NULL){
		return false;
	}

	if(fgets(line, sizeof(line), _pipe) != NULL){

			get_value(line, ':', id);

			if(strcmp(id, "off/any") != 0){
				copy_wo_quotes(ESSID, id);

				found = true;
			}
	}

	pclose(_pipe);

	return found;
}

void etm::Wifi::scan(void)
{
	char line[256], cmd[128];
	char value[64];
	_wifi_ctr = 0;
	uint8_t data = 0;
	uint8_t no_enc, master;
	wifi_entry we;

	_err_num = 0;

	if(!_enabled)
		return;

	sprintf(cmd, "iwlist %s scan | grep -E \"ESSID|Mode|Quality|Encryption\"", _device);

	if((_pipe = popen(cmd, "r")) == NULL){
		_err_num = 3;
		return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){
			switch(data){
			case 0:
				memset(&we, 0, sizeof(we));

				get_value(line, '=', value);

				strcpy(we.quality, value);

				data = 1;
					break;
			case 1:
				get_value(line, ':', value);

				if(strcmp("off", value) == 0)
					no_enc = 1;
				else
					no_enc = 0;

				data = 2;
					break;
			case 2:
				get_value(line, ':', value);

				copy_wo_quotes(we.essid, value);

				data = 3;
					break;
			case 3:
				get_value(line, ':', value);

				if(strcmp("Master", value) == 0)
					master = 1;
				else
					master = 0;

				if(master == 1 && no_enc == 1){
					_entries[_wifi_ctr++] = we;
				}

				data = 0;
					break;
			}
	}

	pclose(_pipe);
}

void etm::Wifi::connect(char *ESSID)
{
	char line[256], cmd[128];


	system("cp /etc/wpa_supplicant/wpa_supplicant.conf.original /etc/wpa_supplicant/wpa_supplicant.conf");

	_err_num = 0;

	if((_fp = fopen("/etc/wpa_supplicant/wpa_supplicant.conf", "a")) == NULL){
		_err_num = 29;
		return;
	}

	fprintf(_fp, "network={\n");
	fprintf(_fp, "\tssid=\"%s\"\n", ESSID);
	fprintf(_fp, "\tkey_mgmt=NONE\n");
	fprintf(_fp, "}\n");

	fclose(_fp);

	sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s reassociate", _device);

	if((_pipe = popen(cmd, "r")) == NULL){
			_err_num = 30;
			return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){
		if(strncmp(line, "OK", 2) == 0)
			break;

		if(strncmp(line, "FAIL", 4) == 0){
			_err_num = 31;
			return;
		}
	}
	pclose(_pipe);

	sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s add_network", _device);

	if((_pipe = popen(cmd, "r")) == NULL){
			_err_num = 32;
			return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){

		if(isdigit(line[0]))
			copy_wo_specs(_network, line);

		if(strncmp(line, "OK", 2) == 0)
			break;

		if(strncmp(line, "FAIL", 4) == 0){
			_err_num = 33;
			return;
		}
	}
	pclose(_pipe);

	sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s set_network %s ssid '\"%s\"'", _device, _network, ESSID);

	if((_pipe = popen(cmd, "r")) == NULL){
			_err_num = 34;
			return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){

		if(strncmp(line, "OK", 2) == 0)
			break;

		if(strncmp(line, "FAIL", 4) == 0){
			_err_num = 35;
			return;
		}
	}
	pclose(_pipe);

	sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s set_network %s scan_ssid 1", _device, _network);

	if((_pipe = popen(cmd, "r")) == NULL){
			_err_num = 36;
			return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){

		if(strncmp(line, "OK", 2) == 0)
			break;

		if(strncmp(line, "FAIL", 4) == 0){
			_err_num = 37;
			return;
		}
	}
	pclose(_pipe);


	sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s set_network %s key_mgmt NONE", _device, _network);

	if((_pipe = popen(cmd, "r")) == NULL){
			_err_num = 38;
			return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){

		if(strncmp(line, "OK", 2) == 0)
			break;

		if(strncmp(line, "FAIL", 4) == 0){
			_err_num = 39;
			return;
		}
	}
	pclose(_pipe);

	sprintf(cmd, "wpa_cli -p /var/run/wpa_supplicant -i %s enable_network %s", _device, _network);

	if((_pipe = popen(cmd, "r")) == NULL){
			_err_num = 40;
			return;
	}

	while(fgets(line, sizeof(line), _pipe) != NULL){

		if(strncmp(line, "OK", 2) == 0)
			break;

		if(strncmp(line, "FAIL", 4) == 0){
			_err_num = 41;
			return;
		}
	}
	pclose(_pipe);

	usleep(100000);

	sprintf(cmd, "udhcpc -i %s -n > /dev/null", _device);

	system(cmd);
}

void etm::Wifi::connect(uint32_t idx)
{
	_err_num = 0;

	if(idx >= _wifi_ctr){
		_err_num = 1;
		return;
	}

	connect(_entries[idx].essid);
}

void etm::Wifi::get_value(char *src, char term, char *value)
{
	uint32_t i, j = 0;

	for(i = 0; src[i] != term; i++);

	for(i++; src[i] != ' ' && src[i] != '\n' && i < strlen(src); i++)
	{
		value[j++] = src[i];
	}
	value[j] = '\0';
}

void etm::Wifi::copy_wo_quotes(char *dest, char *src)
{
	while(*src){

		if(*src == '"'){
			src++;
			continue;
		}

		*dest = *src;

		src++;
		dest++;
	}

	*dest = '\0';
}

void etm::Wifi::copy_wo_specs(char *src, char *dest)
{
	while(*src){

		if(*src == '\n' || *src == '\r'){
			break;
		}

		*dest = *src;

		src++;
		dest++;
	}

	*dest = '\0';
}


void etm::Wifi::get_wifi_name(uint32_t idx, char *buf)
{
	if(idx >= _wifi_ctr)
		return;

	strcpy(buf, _entries[idx].essid);
}

void etm::Wifi::get_ip(char *buf)
{
	char line[256], ip[16], cmd[128];
	sprintf(cmd, "ifconfig %s | grep inet", _device);

	if((_pipe = popen(cmd, "r")) == NULL){
		_err_num = 3;
		return;
	}

	if(fgets(line, sizeof(line), _pipe) != NULL){
		get_value(line, ':', ip);
	}

	pclose(_pipe);

	strcpy(buf, ip);
}



