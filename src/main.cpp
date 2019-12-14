#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <climits>
#include <getopt.h>
#include <errno.h>
#include <sys/inotify.h>

#include "log.h"

void printHelp(){
    std::cout << "Usage:\n";
    std::cout << "\t./cdumpd [options]\n";
    std::cout << "Options:\n";
    std::cout << "\t-d/--debug: Debug Mode - No daemonizing\n";
	std::cout << "\t-h/--help: Print thist Help\n";
    std::cout << "\t-p/--path <path>: Path to coredump/mindump Folder";
}

int main(int argc, char *argv[]){
    static struct option long_options[] = {
		{"debug", no_argument,0, 'd'},
        {"help", no_argument, 0, 'h' },
        {"path", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int opt = 0;
    int option_index = 0;
	bool isDebug = false;
    std::string inotify_path = "./";
    while ((opt = getopt_long(argc, argv,"dhp:", 
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
            case 'p':{
                inotify_path = std::string(optarg);
            }
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
    log->print("Path to Inode: " + inotify_path);
    
    int ifd = inotify_init();
    if(ifd == -1){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not init inotify:\n" + failstr);
        return (-1);
    }
    int wfd = inotify_add_watch(ifd,inotify_path.c_str(), IN_CREATE | IN_CLOSE_WRITE );
    if(wfd == -1){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not add " + inotify_path + " to inotify:\n" + failstr);
        return (-1);
    }
    
    bool running = true;
    while(running){
        const int BUF_LEN = 4096;
        char buffer[BUF_LEN];
        struct inotify_event *event = NULL;
        int len = read(ifd, buffer, BUF_LEN);
        if(len < 0){
            std::string failstr = std::string(strerror(errno));
            log->print("Could read from  inotify fd:\n" + failstr);
            continue;
        }
        event = reinterpret_cast<struct inotify_event *>(buffer);
        while(event != nullptr){
            if((event->mask & IN_CREATE) && (event->len > 0)){
                log->print("File Created: " + std::string(event->name) + "\n");
            }
            if((event->mask & IN_CLOSE_WRITE) && (event->len > 0)){
                log->print("File Write Closed: " + std::string(event->name) + "\n");
            }

            /* Move to next struct */
            len -= sizeof(*event) + event->len;
            if (len > 0)
                event =  event + sizeof(event) + event->len;
            else
                event = NULL;
            }
    }

    if(inotify_rm_watch(ifd, wfd) == -1){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not remove watch from inotify:\n" + failstr);
    }

    close(ifd);
    return 0;
}
