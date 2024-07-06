#ifndef __CONSTS_H_INCLUDED__
#define __CONSTS_H_INCLUDED__


#define MAX_PLAYLIST_NAME_LEN 50        // 
#define PROTOCOL_VERSION    0x0100      // Major/Minor

#define DEFAULT_HTTPD_PORT  8080
#define DEFAULT_HTTPD_ADDR  INADDR_ANY


constexpr auto VAR_REGEX = "[$|%]\\((?<varb>[a-z]+[a-z0-9_-]+)\\)|\\{(?<varc>[a-z]+[a-z0-9_-]+)\\}";


#endif
