#ifndef __READCONFIG_H__
#define __READCONFIG_H__

#include <stdio.h>
#include <map>
#include <string>

#define CONFIG_LINE_BUFFER_SIZE 256

struct comparer
{
    public:
    bool operator()(const std::string x, const std::string y)
    {
         return x.compare(y)<0;
    }
};


class ReadConfig{
private:
	bool _is_open;
	int _err_num;

	std::map<std::string, double, comparer> _configlist;
	std::map<std::string, double, comparer>::iterator _iterator;

public:
	ReadConfig(char* filename);
	~ReadConfig(void);

	double GetParam(std::string _key);
	double GetParam(std::string _key, double defaultvalue);

};



#endif
