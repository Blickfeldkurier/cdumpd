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

//http_upload is from the google_breakpad library.
//since it is not installed in Path, it was copied 
//and modified to fit the Programm structure
#include "http_upload.h"

//Wrapper arround http_upload.h
using namespace google_breakpad;
namespace {
    /**
     * Upload a Minidumpfile to given Url.
     * Takes a Filepath and uploads it to the given url.
     * On Error it returns False and sets the string given by error.
     * @param path Path to the file we want to upload
     * @param url Url to upload to
     * @param error Contains the error text
     * @return bool False on error
     **/
    bool uploadMinidump(std::string &path, std::string &url, std::string *error) {
        std::map<string, string> parameters;
        std::map<string, string> files;
        files["upload_file_minidump"] = path;
        return HTTPUpload::SendRequest(
        url,
        parameters,
        files,
        /* proxy */ "",
        /* proxy password */ "",
        /* certificate */ "",
        /* response body */ nullptr,
        /* response code */ nullptr,
        error
        );
    }
}

/**
 * Delete a File if the upload was succssesfull
 * @param path Path to the file we want to be deleted
 * @param noError Return value of the File upload. True == Upload succssesfull
 * @param errmsg Error String from upload, might contain the reason for failure
 * @param log Pointer to the Log object we want to write the error messages to. 
 */ 
void deleteFile(std::string &path, bool noError, std::string *errmsg, Log *log){
    if(noError == false){
        log->print("Could not upload " + path + ": " + *errmsg + "\n");
        return;
    }
    int retval = remove(path.c_str());
    if(retval != 0){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not delete " + path + ":\n" + failstr);
    }
}

void printHelp(){
    std::cout << "Usage:\n";
    std::cout << "\t./cdumpd [options]\n";
    std::cout << "Options:\n";
	std::cout << "\t-c/--contains <string>: Match filename agains string\n";
    std::cout << "\t-d/--debug: Debug Mode - No daemonizing\n";
	std::cout << "\t-h/--help: Print thist Help\n";
    std::cout << "\t-p/--path <path>: Path to coredump/mindump Folder\n";
    std::cout << "\t-u/--url <url>: Upload url (default: http://localhost:8080)\n";
}

int main(int argc, char *argv[]){
    //Structure for long_options
    static struct option long_options[] = {
		{"contains", required_argument, 0, 'c'},
		{"debug", no_argument,0, 'd'},
        {"help", no_argument, 0, 'h' },
        {"path", required_argument, 0, 'p'},
        {"url", required_argument, 0, 'u'},
        {0, 0, 0, 0}
    };

    //Option Parsing
    int opt = 0;
    int option_index = 0;
	bool isDebug = false;
    std::string inotify_path = "./";
    std::string url = "http://localhost:8080";
	std::string contains = "";
    while ((opt = getopt_long(argc, argv,"c:dhp:u:", 
                   long_options, &option_index )) != -1) 
    {
        switch(opt){
			case 'c':{//Pattern for filetest
				contains = std::string(optarg);
			}
			case 'd':{//Flag for Debug mode
				isDebug = true;
			}break;
            case 'h':{//Print help
                printHelp();
                exit(0);
            }break;
            case 'p':{//Path to Folder we want to watch
                inotify_path = std::string(optarg);
            }break;
            case 'u':{//Upload Url
                url = std::string(optarg);
            }break;
            case '?':{
                std::cout << "Could not find argument. You you want some --help\n";
                exit(1);
            }
        }
    }

	if(isDebug == false){
		if(daemon(0,0) != 0){
            std::string failstr = std::string(strerror(errno));
            std::cout << ("Could not daemonize:\n" + failstr);
            exit(1);
        }
	}
	Log *log = new Log(isDebug);
    log->print("Path to Inode: " + inotify_path + "\n", "");
    log->print("Upload to: " + url + "\n", "");

    //Setup Inotify
    int ifd = inotify_init();
    if(ifd == -1){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not init inotify:\n" + failstr);
        return (-1);
    }
    int wfd = inotify_add_watch(ifd,inotify_path.c_str(), IN_CLOSE_WRITE );
    if(wfd == -1){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not add " + inotify_path + " to inotify:\n" + failstr);
        return (-1);
    }
    
    //Mainloop
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
        //Process Inotify Events
        event = reinterpret_cast<struct inotify_event *>(buffer);
        while(event != nullptr){
            if((event->mask & IN_CLOSE_WRITE) && (event->len > 0)){
                std::string evname = std::string(event->name);
                std::string path = inotify_path + "/" + evname;
                std::string *error = new std::string();
                log->print("File Write Closed: " + evname + "\n", "");
                if(contains.empty() == true){
					bool retval = uploadMinidump(path, url, error);
                    deleteFile(path, retval, error, log);
                }else{
                    if(contains.find(evname) != std::string::npos){
                        bool retval = uploadMinidump(path, url, error);
                        deleteFile(path, retval, error, log);
                    }
                }
                delete(error);
			}
            //Move to next struct
            len -= sizeof(*event) + event->len;
            if (len > 0)
                event =  event + sizeof(event) + event->len;
            else
                event = NULL;
            }
    }

    //Celeanup
    if(inotify_rm_watch(ifd, wfd) == -1){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not remove watch from inotify:\n" + failstr);
    }

    close(ifd);
    return 0;
}
