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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "sentry_fields.h"
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
    bool uploadMinidump(std::string &path, std::string &url, 
                        bool hasSentry, Log* log, bool debug) 
    {
        std::map<string, string> parameters;
        std::map<string, string> files;
        SentryFields *sentry = nullptr;
        if(hasSentry){
            sentry = new SentryFields(log);
            parameters = sentry->getParams();
        }

        long responce;
        std::string body;
        std::string lerror;

        files["upload_file_minidump"] = path;
        bool succsess = HTTPUpload::SendRequest(
        url,
        parameters, 
        files,
        /* proxy */ "",
        /* proxy password */ "",
        /* certificate */ "",
        /* response body */ &body,
        /* response code */ &responce,
        &lerror,
        debug
        );
        if(hasSentry){
            delete(sentry);
            sentry = nullptr;
        }
        if(succsess == false){
            log->print("Could not Upload " + path + " to: " + url + "\n");
            log->print("Error:\n\nCode: " + std::to_string(responce) + "\n\tBody:\n" + body + +"\n\terror:\n" + lerror + "\n" );
            if(parameters.empty() == false){
                map<string, string>::const_iterator iter = parameters.begin();
                log->print("Parameters:\n");
                for (; iter != parameters.end(); ++iter){
                    log->print(iter->first + " = " + iter->second + "\n");
                }
            }
        }
        return succsess;
    }
}

/**
 * Delete a File if the upload was succssesfull
 * @param path Path to the file we want to be deleted
 * @param noError Return value of the File upload. True == Upload succssesfull
 * @param errmsg Error String from upload, might contain the reason for failure
 * @param log Pointer to the Log object we want to write the error messages to. 
 */ 
void deleteFile(std::string &path, bool noError, Log *log){
    if(noError == false){
        log->print("Could not upload " + path + "\n");
        return;
    }
    int retval = remove(path.c_str());
    if(retval != 0){
        std::string failstr = std::string(strerror(errno));
        log->print("Could not delete " + path + ":\n" + failstr);
    }
}

/**
 * Wrapper around Pattern Check, File Upload and deletion
 */
void fileProcessor(std::string dirpath, std::string filename, std::string contains, std::string url, 
                   bool hazSentry, bool isDebug, Log *log)
{
    std::string path = dirpath + filename;
    if(contains.empty() == true){
		bool retval = uploadMinidump(path, url, hazSentry, log, isDebug);
        deleteFile(path, retval, log);
    }else{
		log->print("Test for " + contains + " in " + filename , "");
        if(filename.find(contains) != std::string::npos){
        bool retval = uploadMinidump(path, url, hazSentry, log, isDebug);
        deleteFile(path, retval, log);                }
    }
}

/**
 * Get all the filenames in INODE-Directory and check for leftovers
 * If Found Upload them
 */
void startupCheck(std::string path, std::string url, std::string pattern, bool isSentry, bool isDebug, Log*log){
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != nullptr) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != nullptr) {
            struct stat path_stat;
            std::string filename = std::string(ent->d_name);
            std::string filepath = path + "/" + filename;
            if(stat(filepath.c_str(), &path_stat) < 0){
                std::string failstr = std::string(strerror(errno));
                log->print("Could not stat " + filepath + ":\n" + failstr);
                continue;
            }
            if((path_stat.st_mode & S_IFMT) != S_IFREG){
                continue;
            }
            if(filename.compare(".") == 0 || filename.compare("..") == 0){
                continue;
            }
            fileProcessor(path + "/", filename, pattern, url, isSentry, isDebug, log);
        }
        closedir (dir);
    } else {
        std::string failstr = std::string(strerror(errno));
        log->print("Skipping Upload of old Files. Could not open Dir: " + path + ":\n" + failstr);
    }

}

void printHelp(){
    std::cout << "Usage:\n";
    std::cout << "\t./cdumpd [options]\n";
    std::cout << "Options:\n";
	std::cout << "\t-c/--contains <string>: Match filename agains string\n";
    std::cout << "\t-d/--debug: Debug Mode - No daemonizing\n";
	std::cout << "\t-h/--help: Print thist Help\n";
    std::cout << "\t-n/--no-cleanup: Do not send remaining Files on Programm start\n";
    std::cout << "\t-p/--path <path>: Path to coredump/mindump Folder\n";
    std::cout << "\t-s/--sentry: Provide Sentry User/Device Fields in Upload\n";
    std::cout << "\t-u/--url <url>: Upload url (default: http://localhost:8080)\n";
}

int main(int argc, char *argv[]){
    //Structure for long_options
    static struct option long_options[] = {
		{"contains", required_argument, 0, 'c'},
		{"debug", no_argument,0, 'd'},
        {"help", no_argument, 0, 'h' },
        {"no-cleanup", no_argument, 0, 'n'},
        {"path", required_argument, 0, 'p'},
        {"sentry", no_argument, 0, 's'},
        {"url", required_argument, 0, 'u'},
        {0, 0, 0, 0}
    };

    //Option Parsing
    int opt = 0;
    int option_index = 0;
	bool isDebug = false;
    bool iCanHazSentryFields = false;
    bool no_cleanup = false;
    std::string inotify_path = "./";
    std::string url = "http://localhost:8080";
	std::string contains = "";
    while ((opt = getopt_long(argc, argv,"c:dhnp:su:", 
                   long_options, &option_index )) != -1) 
    {
        switch(opt){
			case 'c':{//Pattern for filetest
				contains = std::string(optarg);
			}break;
			case 'd':{//Flag for Debug mode
				isDebug = true;
			}break;
            case 'h':{//Print help
                printHelp();
                exit(0);
            }break;
            case 'n':{
                no_cleanup = true;
            }break;
            case 'p':{//Path to Folder we want to watch
                inotify_path = std::string(optarg);
            }break;
            case 's':{
                iCanHazSentryFields = true;
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
    log->print("Path to Inotify: " + inotify_path + "\n");
    log->print("Upload to: " + url + "\n");

    if(no_cleanup == false){
        startupCheck(inotify_path, url, contains, iCanHazSentryFields, isDebug, log);
    }

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
                log->print("File Write Closed: " + evname + "\n", "");
                fileProcessor(inotify_path + "/" + evname, evname, contains, url, iCanHazSentryFields, isDebug, log);
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
