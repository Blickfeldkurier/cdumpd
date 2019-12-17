#include "sentry_fields.h"

SentryFields::SentryFields(Log *log){
    this->log = log;
    this->getUname();
    this->getIPAddress();
    this->getSysinfo();
}

void SentryFields::getIPAddress(){
    this->hazIpAddr = false;
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
    if(this->hazIpAddr == true){
        retMap.insert({sentry + "[\"user\"][\"ip_address\"]", this->ipAddress});
    }
    if(this->hazSysinfo == true){
        retMap.insert({sentry + "[\"contexts\"][\"device\"][\"memory_size\"]", std::to_string(this->sinfo.totalram)}); //in Bytes
        retMap.insert({sentry + "[\"contexts\"][\"device\"][\"free_memory\"]", std::to_string(this->sinfo.freeram)}); //in Bytes
        retMap.insert({sentry + "[\"contexts\"][\"device\"][\"boot_time\"]", ""});//In Utc - "2018-02-08T12:52:12Z" this->sinfo.uptime
    } 
    if(this->hazUname == true){
        retMap.insert({sentry + "[\"contexts\"][\"device\"][\"name\"]", "\"" + std::string(this->ustruct.nodename) + "\"" }); //unmame -n nodename
        retMap.insert({sentry + "[\"contexts\"][\"device\"][\"arch\"]", "\"" + std::string(this->ustruct.machine) + "\""}); // uname -m machine
        retMap.insert({sentry + "[\"contexts\"][\"os\"][\"name\"]", "\"" + std::string(this->ustruct.sysname) + "\""}); //uanme -s sysname
        retMap.insert({sentry + "[\"contexts\"][\"os\"][\"kernel_version\"]", "\"" + std::string(this->ustruct.release) + "\""}); //uname -r release
        retMap.insert({sentry + "[\"contexts\"][\"os\"][\"version\"]", "\"" + std::string(this->ustruct.version) + "\""}); //uname version
    }
    return retMap;
}