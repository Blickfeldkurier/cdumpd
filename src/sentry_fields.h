#ifndef SENTRY_FIELDS_H
#define SENTRY_FIELDS_H

#include <string>
#include <algorithm>
#include <map>
#include <sys/utsname.h>
#include <errno.h>
#include <cstring>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <ctime>
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include "log.h"

class SentryFields{

private:
    std::string ipAddress;
    struct utsname ustruct;
    struct sysinfo sinfo;
    Log *log;
    bool hazUname;
    bool hazSysinfo;
    bool hazIpAddr;

    void getIPAddress(std::string hostname);
    void getSysinfo();
    void getUname();

public:
    SentryFields(Log *log);
    std::map<std::string, std::string> getParams();
};

#endif