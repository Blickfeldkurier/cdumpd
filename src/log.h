#ifndef DTOID_LOG_H
#define DTOID_LOG_H

#include <string>
#include <iostream>
#include <syslog.h>

class Log {
public:
	Log(bool debug);
	void print(std::string text, int level = LOG_INFO );
	void print(std::string debug_txt, std::string log_txt, int level = LOG_INFO );
private:
	bool debug;

};

#endif
