#include "ReadConfig.h"


ReadConfig::ReadConfig(char* filename)
{
	FILE* _fileid;

	_is_open = false;
	_err_num = 0;

	_fileid = fopen(filename,"r");
	if(_fileid == NULL){
		_err_num = 1;
		return;
	}

	char buf[CONFIG_LINE_BUFFER_SIZE];
	char key[CONFIG_LINE_BUFFER_SIZE];
	double value;

	fseek(_fileid, 0, SEEK_SET);
	while(! feof(_fileid)) {
		fgets(buf, CONFIG_LINE_BUFFER_SIZE, _fileid);

		if (buf[1] != '#')
		{
			sscanf(buf, "%s %lf", key, &value);
			_configlist.insert(std::pair<std::string, double>(key, value));				
		}
	}

	_is_open = true;
	fclose(_fileid);
}

ReadConfig::~ReadConfig(void)
{;}

double ReadConfig::GetParam(std::string _key)
{
	_err_num = 0;

	if (!_is_open)
	{
		_err_num = 2;
		return 0.0;
	}

	_iterator = _configlist.find( _key);
	if(_iterator != _configlist.end())
	{
		return (*_iterator).second;
	}
	
	// not found
	_err_num = 3;
	return 0.0;
}

double ReadConfig::GetParam(std::string _key, double defaultvalue)
{
	_err_num = 0;

	if (!_is_open)
	{
		_err_num = 2;
		return defaultvalue;
	}

	_iterator = _configlist.find(_key);
	if(_iterator != _configlist.end())
	{
		return (*_iterator).second;
	}
	
	// not found
	_err_num = 3;
	return defaultvalue;
}

