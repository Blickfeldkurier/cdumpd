# Cdumpd - Watch folder for Coredumps and upload them to a Webservice

## Dependencies
 * Curl

## Build
```
 mkdir build
 cd build
 cmake ../
 make package
```

## Start from Systemd - CdumpdExample.service

```
[Unit]
Description=Watch for coredumps and upload them to a Webservice
After=network-online.target

[Service]
Type=forking
ExecStart=/usr/usr/local/bin/cdumpd -p /srv/coredumps -u http://localhost:8080 -c .dmp

[Install]
WantedBy=multi-user.target
```
