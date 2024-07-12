#ifndef __FTP_H_INCLUDED__
#define __FTP_H_INCLUDED__


#include <stdint.h>


extern int ftp_start_server(struct sockaddr_in *addr);
extern int ftp_stop_server(void);
extern void ftp_serve(void);


#endif
