#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <climits>
#include <getopt.h>

#include "log.h"

void printHelp(){
    std::cout << "Usage:\n";
    std::cout << "\t./dumpd [options]\n";
    std::cout << "Options:\n";
    std::cout << "\t-d/--debug: Debug Mode - No daemonizing\n";
	std::cout << "\t-h/--help: Print thist Help\n";
}

int main(int argc, char *argv[]){
    static struct option long_options[] = {
		{"debug", no_argument,0, 'd'},
        {"help", no_argument, 0, 'h' },
        {0, 0, 0, 0}
    };

    int opt = 0;
    int option_index = 0;
	bool isDebug = false;
    std::string config_path = "./test.conf";
    while ((opt = getopt_long(argc, argv,"dh", 
                   long_options, &option_index )) != -1) 
    {
        switch(opt){
			case 'd':{
				isDebug = true;
			}break;
            case 'h':{
                printHelp();
                exit(0);
            }break;
            case '?':{
                std::cout << "Could not find argument. You you want some --help\n";
                exit(1);
            }
        }
    }
	if(isDebug == false){
		daemon(0,0);	
	}
	Log *log = new Log(isDebug);

    bool running = true;
    while(running){
        
    }

    return 0;
}
