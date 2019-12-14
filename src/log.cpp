#include "log.h"
Log::Log(bool debug){
	this->debug = debug;
}

void Log::print(std::string text, int level){
	this->print(text, text, level);
}

void Log::print(std::string debug_txt, 
				std::string log_txt,
				int level)
{
	if(debug == true){
		std::cout << debug_txt << "\n";
	}else{
		syslog(level, "%s",log_txt.c_str());
	}
}

