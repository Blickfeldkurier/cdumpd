#include "sentry_fields.h"

SentryFields::SentryFields(Log *log){
    this->log = log;
    this->getUname();
    this->getSysinfo();
}

void SentryFields::getIPAddress(std::string hostname){
    this->hazIpAddr = false;
    struct hostent *host_entry; 
    host_entry = gethostbyname(hostname.c_str());
    if(host_entry == nullptr){
        log->print("Could not gethostbyname: " + std::string(strerror(errno)));
        return;
    }
    this->ipAddress = std::string(inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])));
    if(this->ipAddress.empty() == false){
        this->hazIpAddr = true;
    }
}

void SentryFields::getSysinfo(){
    this->hazSysinfo = false;
    int retval = sysinfo(&this->sinfo);
    if(retval < 0){
        log->print("Could not get sysinfo: " + std::string(strerror(errno)));
        return;
    }
    this->hazSysinfo = true;
}

void SentryFields::getUname(){
    this->hazUname = false;
    int retval = uname(&this->ustruct);
    if(retval < 0){
        log->print("Could not get uname: " + std::string(strerror(errno)));
        return;
    }
    this->hazUname = true;
}

std::map<std::string, std::string> SentryFields::getParams(){
    std::map<std::string, std::string> retMap;
    std::string sentry = "sentry";
    std::string contextString = "\"contexts\":{\"device\":{";
    if(this->hazSysinfo == true){
        std::time_t uptime = this->sinfo.uptime;
        time_t base = time(0);
        std::time_t delta_t = base - uptime;
        std::string localtime = std::string(ctime(&delta_t));
        localtime.erase(std::remove(localtime.begin(), localtime.end(), '\n'), localtime.end());
        contextString = contextString + "\"memory_size\": " +  std::to_string(this->sinfo.totalram) + ", \"free_memory\": " + std::to_string(this->sinfo.freeram) +", \"boot_time\": \"" + localtime + "\"";
    }
    if(this->hazUname == true){
        if(this->hazSysinfo == true){
            contextString = contextString + ",";
        }
        contextString = contextString + "\"name\": \"" + std::string(this->ustruct.nodename) + "\", \"arch\": \"" + std::string(this->ustruct.machine) + "\"" ;
    }
    contextString = contextString + "}"; //Closing for device Section
    if(this->hazUname){
        contextString = contextString + ",\"os\":{";
        contextString = contextString + + "\"name\": \"" + std::string(this->ustruct.sysname) + "\"," ;
        contextString = contextString + + "\"kernel_version\": \"" + std::string(this->ustruct.release)  + "\"," ;
        contextString = contextString + + "\"version\": \"" + std::string(this->ustruct.version)  + "\"" ;
        contextString = contextString + "}";
    }
    contextString = contextString + "}"; //Closing for contexts Section
    std::string userString = "\"user\":{";
    if(this->hazUname){
        this->getIPAddress(std::string(this->ustruct.nodename));
        userString = userString + "\"ip_address\":\"" + this->ipAddress + "\"";
    }
    userString = userString + "}";
    retMap.insert({"sentry", "{" + contextString + "," + userString + "}"});
    return retMap;
}