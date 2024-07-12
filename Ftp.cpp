#include "ftp.h"

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>


extern void UART_Printf(const char *, ...);


#if 0
int errno = 0;
#endif


/* Port range for passive mode. Choosed in random manner.
 */
#define FTP_CLIENT_PORT_RANGE_MIN   44000
#define FTP_CLIENT_PORT_RANGE_MAX   45000


/* Maximum length of user name string
 */
#define MAX_FTP_USERNAME    16


/* Maximum number of FTP clients that can be served simultaneously
 */
#define MAX_FTP_CLIENTS     4


#undef FTP_ALLOW_ANONYMOUS_LOGINS
#define FTP_REQUIRE_AUTH


#define MAX_MSG_LEN 1024


enum {
    FTP_SERVE_CMD,
    FTP_SERVE_DATA
};


typedef enum _ftp_mode
{
    FTP_MODE_ACTIVE,
    FTP_MODE_PASSIVE
} ftp_mode_t;


typedef enum _ftp_dir
{
    FTP_NONE = 0,
    FTP_RETR,
    FTP_STOR
} ftp_dir_t;


#ifndef USE_LWIP_SOCKETS
static void ftp_thread_main(const void *);
static void ftp_client_main(const void *);


osThreadDef(ftp_thread, ftp_thread_main, osPriorityNormal, 40, 2048);
osThreadDef(ftp_client, ftp_client_main, osPriorityNormal, 40, 4096);
#endif
 

#define PATH_MAX    256


typedef struct _ftp_client {
    char free;                  // 0 = slot used, else free
    ftp_mode_t mode;            // Active or Passive mode
    int cmd_sock;
    int data_sock;
    int pass_sock;
    struct sockaddr_in addr;
    char curdir[PATH_MAX];      // Current directory
    ftp_dir_t dir;              // Direction of file transfer
    FIL file;                   // Current file
    char file_opened;           // 1 if file opened for writing
    uint32_t fsize;             // Current bytes transferred
    uint32_t total;             // Total bytes transferred
    time_t t_start;             // Operation start time
    char user[MAX_FTP_USERNAME];// User name
    char authed;                // 1 if user authenticated, 0 otherwise
//    struct _ftp_client *next; // Pointer to the next structure in chain
} ftp_client_t;


typedef struct _ftp {
#ifdef USE_LWIP_SOCKETS
    int listen_sock;
#else    
    osThreadId thread_id;       // RTOS thread ID
    struct netconn *nc;         // Listening socket
#endif    
    char require_auth;          // Require authentication
    char allow_anonymous;       // Allow anonymous login
    int nclients;               // Number of clients connected
    ftp_client_t clients[MAX_FTP_CLIENTS];
} ftp_t;


typedef void (*ftp_command_fn)(ftp_client_t *, const char *, uint16_t);


struct _ftp_cmd {
    char len;
    const char *kw;
    ftp_command_fn fn;
};


extern const char *ff_errstr(int);


#define IO_BUF_SIZE 1460*4


static char _iobuf[IO_BUF_SIZE];

/* I plan to #define macro to get rid of this stuff
 */
static void cmd_syst(ftp_client_t *, const char *, uint16_t);
static void cmd_user(ftp_client_t *, const char *, uint16_t);
static void cmd_pass(ftp_client_t *, const char *, uint16_t);
static void cmd_list(ftp_client_t *, const char *, uint16_t);
static void cmd_quit(ftp_client_t *, const char *, uint16_t);
static void cmd_port(ftp_client_t *, const char *, uint16_t);
static void cmd_pasv(ftp_client_t *, const char *, uint16_t);
static void cmd_type(ftp_client_t *, const char *, uint16_t);
static void cmd_pwd(ftp_client_t *, const char *, uint16_t);
static void cmd_cwd(ftp_client_t *, const char *, uint16_t);
static void cmd_cdup(ftp_client_t *, const char *, uint16_t);
static void cmd_opts(ftp_client_t *, const char *, uint16_t);
static void cmd_put(ftp_client_t *, const char *, uint16_t);
static void cmd_get(ftp_client_t *, const char *, uint16_t);
static void cmd_size(ftp_client_t *, const char *, uint16_t);
static void cmd_feat(ftp_client_t *, const char *, uint16_t);
static void cmd_dele(ftp_client_t *, const char *, uint16_t);


/* This is short subset of FTP protocol commands, supported
 * by our FTP server. Other commands will be answered with
 * status code "502 Command not implemented"
 */
struct _ftp_cmd _ftp_cmds[] = {
    { 4, "USER", cmd_user },    // Login
    { 4, "PASS", cmd_pass },    // Password
    { 4, "SYST", cmd_syst },    // System type
    { 4, "PORT", cmd_port },    // Enter active mode
    { 4, "TYPE", cmd_type },    // Set transfer type
    { 4, "PASV", cmd_pasv },    // Enter passive mode
    { 4, "LIST", cmd_list },    // List current dir contents
    { 3, "PWD",  cmd_pwd  },    // Print current directory
    { 4, "OPTS", cmd_opts },    // Set various options 
    { 3, "CWD",  cmd_cwd  },    // Change current directory
    { 4, "CDUP", cmd_cdup },    // CD 1 level up
    { 3, "RETR", cmd_get  },    // Download file from FTP server
    { 3, "STOR", cmd_put  },    // Upload file to FTP server
    { 4, "SIZE", cmd_size },    // Obtain file size
    { 4, "FEAT", cmd_feat },    // Show list of supported extensions
    { 4, "TYPE", NULL },        // Set transfer type: binary or ascii
    { 3, "BIN",  NULL },        // Set trfanfer type to binary
    { 4, "QUIT", cmd_quit },    // End session
    { 4, "DELE", cmd_dele },    // Delete file/directory
    { 3, "MKD",  NULL },        // Make directory
    { 3, "RMD",  NULL },        // Remove directory
    { 4, "RNTO", NULL },        // Rename file
    { 4, "NOOP", NULL },        // Do nothing
};


static char _banner[] = "Welcome to the FTP micro server.\r\n200 (c) 2010-2011 KSC Elcom, LLC.";
static char _system_type[] = "KSC-BND24-3 (CMSIS RTOS)";


ftp_t _ftp;
#ifndef USE_LWIP_SOCKETS
osMutexDef(_ftp_mtx_def);
static osMutexId _ftp_mtx;
#endif


static time_t
time(time_t *t)
{
    return 0;
}


/* Replacement for missing function inet_ntoa()
 */
static const char *
_inet_ntoa(struct in_addr *addr)
{
    static char s_ip[32];
    unsigned char *octs = (unsigned char *)&addr->s_addr;
    snprintf(s_ip, sizeof(s_ip), "%u.%u.%u.%u", octs[0], octs[1], octs[2], octs[3]); 
    return s_ip;
}


/* Send message to specified client
 */
static void 
msg(ftp_client_t *c, int code, const char *msg, ...)
{
    static char tmp[MAX_MSG_LEN+3];
    va_list args;
    int len;
    
    if(code > 0) {
        len = snprintf(tmp, MAX_MSG_LEN, "%d ", code);
    } else {
        len = 0;
    }
    
    va_start(args, msg);
    len += vsnprintf(tmp+len, MAX_MSG_LEN-len, msg, args);
    va_end(args);
    
    strcat(tmp, "\r\n");
    len += 2;

    tmp[len] = 0;
    UART_Printf("ftpd: %s", tmp);
    
#ifdef USE_LWIP_SOCKETS    
    lwip_send(c->cmd_sock, tmp, len, 0);
#else
    netconn_write(c->cmd_conn, tmp, len, NETCONN_NOFLAG);
#endif    
}


/* Returns ID of matched FTP command
 */
static void
exec_command(ftp_client_t *fc, char *text, uint16_t cmdlen)
{
    int i;
    struct _ftp_cmd *cmd;
    
    text[cmdlen-2] = 0; // cut off CRLF
    UART_Printf("ftpd: command: \"%s\"\r\n", text);
    
    for(i=0; i<sizeof(_ftp_cmds)/sizeof(_ftp_cmds[0]); ++i) {
        cmd = &_ftp_cmds[i];
        
        if(cmdlen <= cmd->len) {
            continue;
        }
        
        if(NULL == cmd->fn) {
            continue;
        }
        
        if(0 == strncasecmp("LIST", text, 4)) {
            int stop = 1;
            fc->authed = stop;
        }
        
        if(0 == strncasecmp(cmd->kw, text, cmd->len)) {
            //if(' ' == text[cmd->len] || '\r' == text[cmd->len]) 
            {
                text += cmd->len;
                cmdlen -= cmd->len;
                
                while(' '== *text || '\r' == *text || '\n' == *text || '\t' == *text) {
                    ++text;
                    --cmdlen;
                }
                    
                cmd->fn(fc, text, cmdlen);
                return;
            }
        }
    }
    
    UART_Printf("Command \"%s\" not implemented yet\r\n", text);
    msg(fc, 502, "Command not implemented");
}


static ftp_client_t *
new_client(ftp_t *ftp)
{
    int i;
    ftp_client_t *c;
    
    for(i=0; i<MAX_FTP_CLIENTS; ++i) {
        if(ftp->clients[i].free) {
            break;
        }
    }
    
    if(i<MAX_FTP_CLIENTS) {
        /* Found free slot 
         * Initialize it and return address to caller
         */
        c = &ftp->clients[i];
        
        c->free = 0;
        c->mode = FTP_MODE_ACTIVE;
        c->cmd_sock = -1;
        c->data_sock = -1;
        c->pass_sock = -1;
        strcpy(c->curdir, "/");
        c->dir = FTP_NONE;
        memset(&c->file, 0, sizeof(c->file));
        c->file_opened = 0;
        c->fsize = 0;
        c->total = 0;
        memset(&c->user[0], 0, sizeof(c->user));
        c->authed = ftp->require_auth? 0: 1;
        
        ++ftp->nclients;

        return c;
    }
    
    return NULL;
}


/* Frees client, releasing memory, closing file, netconns etc.
 */
static void
free_client(ftp_t *ftp, ftp_client_t *c)
{
    if(!c->free) {
        c->free = 1;

        if(FTP_STOR == c->dir) {
            f_sync(&c->file);
        }
        
        f_close(&c->file);
        
        --ftp->nclients;
    }
}


static void
print_banner(ftp_client_t *cli)
{
    msg(cli, 200, _banner);
    msg(cli, 220, "You are %d of %d users allowed.", _ftp.nclients, MAX_FTP_CLIENTS);
}


int ftp_start_server(struct sockaddr_in *addr)
{
    int i;
    
    _ftp.nclients = 0;

    for(i=0; i<MAX_FTP_CLIENTS; ++i) {
        _ftp.clients[i].free = 1;
    }

    UART_Printf("ftpd: Listening on %s:%u\r\n", _inet_ntoa(&addr->sin_addr), addr->sin_port);
    
#ifdef FTP_REQUIRE_AUTH
    _ftp.require_auth = 1;
#ifdef FTP_ALLOW_ANONYMOUS_LOGINS     
    _ftp.allow_anonymous = 1;
#else
    _ftp.allow_anonymous = 0;
#endif
#else
    _ftp.require_auth = 0;
    _ftp.allow_anonymous = 0; // No effect, but let's clear it just in case
#endif
    
    _ftp.listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(_ftp.listen_sock < 0) {
        UART_Printf("ftpd: Cannot create listening socket. Error was %d\r\n", errno);
        return -1;
    }
    
    addr->sin_port = htons(addr->sin_port); 
    addr->sin_family = AF_INET;
    addr->sin_len = sizeof(addr);
    
    if(bind(_ftp.listen_sock, (const struct sockaddr *)addr, sizeof(*addr)) < 0) {
        UART_Printf("ftpd: Cannot bind on specified address. Error was %u\r\n", errno);
        return -1;
    }
    
    if(listen(_ftp.listen_sock, MAX_FTP_CLIENTS+1) < 0) {
        UART_Printf("ftpd: listen() failed. Error was %d\r\n", errno);
        return -1;
    }
    
    return 0;
}


int
ftp_stop_server(void)
{
    int i;
    
    for(i=0; i<MAX_FTP_CLIENTS; ++i) {
        free_client(&_ftp, &_ftp.clients[i]);
    }
    
    return 0;
}


static void
kill_client(ftp_client_t *fc)
{
    free_client(&_ftp, fc);
}


static void 
cmd_syst(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 220, _system_type);
}


static void 
cmd_user(ftp_client_t *fc, const char *args, uint16_t len)
{
    if(0 == strcmp(args, "anonymous")) {
        if(0 == _ftp.allow_anonymous) {
            msg(fc, 332, "Anonymous logins not allowed");
        } else {
            UART_Printf("ftdp: Logged in as anonymous user\r\n");
            msg(fc, 230, "Logged in");
        }
        return;
    }
    
    strncpy(fc->user, args, sizeof(fc->user)-1);
    msg(fc, 331, "Password required for %s.", fc->user);
}


static void 
cmd_pass(ftp_client_t *fc, const char *args, uint16_t len)
{
    if(0 == strlen(fc->user)) {
        msg(fc, 332, "Need account to log in");
        return;
    }
    
    if(0 == strcmp(args, "oem")) { // Passwords are case-sensitive
        fc->authed = 1;
        msg(fc, 220, "You logged as %s", fc->user);
        return;
    }
    
    msg(fc, 430, "Invalid username or password");
}


static void 
cmd_quit(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 220, "Goodbye");
    UART_Printf("ftpd: Client exited. Thread terminated\r\n");
    kill_client(fc);
}


static void 
cmd_opts(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 200, "Ok");
}


static void 
cmd_cwd(ftp_client_t *fc, const char *args, uint16_t len)
{
    if('/' == args[0]) {
        strcpy(fc->curdir, args);
    } else {
        strcat(fc->curdir, args);
    }
    
    msg(fc, 250, "CWD command successful");
}


static void 
cmd_cdup(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 250, "CWD command successful");
}


static char _monthes[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


static void
_format_date(char *dst, uint16_t cb, uint16_t date, uint16_t time)
{
    // date bit15:9 Year origin from 1980 (0..127) bit8:5 Month (1..12) bit 4:0 Day (1..31)
    // time bit15:11 Hour (0..23) bit10:5 Minute (0..59) bit4:0 Second / 2 (0..29)
    uint16_t y, m, d;
    
    y = ((date & 0xFE00) >> 9) + 1980;
    m = (date & 0x01E0) >> 5;
    d = (date & 0x001F);
    
    snprintf(dst, cb, "%s %2u %4u", _monthes[m-1], d, y); 
}


static void
cmd_size(ftp_client_t *fc, const char *args, uint16_t len)
{
    u32_t fs;
    FILINFO fi;
    
    if(ERR_OK == f_stat(args, &fi)) {
        fs = fi.fsize;
        msg(fc, 213, "%d", fs);
    } else {
        msg(fc, 550, "File unavailable");
    }
}


static void
cmd_feat(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 211, "Features supported");
    msg(fc, 0, "SIZE");
    msg(fc, 0, "PASV");
    msg(fc, 211, "END");
}


static void 
cmd_list(ftp_client_t *fc, const char *args, uint16_t len)
{
    DIR dir;
    FILINFO file;
    FRESULT res;
    char d, temp[256], date[64];
    int cb;
    
    UART_Printf("ftpd: Sending directory contents of %s\rЗа\n", fc->curdir);
    
    msg(fc, 150, "Here comes the directory listing");

    set_drive(1);
    res = f_opendir(&dir, "1:/" /*fc->curdir*/);
    if(ERR_OK == res) {
        do {
            set_drive(1);
            res = f_readdir(&dir, &file);
            if(0 == file.fname[0]) {
                break;
            }
            
            memset(&temp, 0, sizeof(temp));
            
            d = (file.fattrib & AM_DIR)? 'd': '-';

            _format_date(date, sizeof(date), file.fdate, file.ftime);
            
            cb = snprintf(temp, sizeof(temp), "%c%s %3d %s %-8lu %s %s\r\n", 
                d, "rwxrw-rw-", 2, "root", file.fsize, date, file.fname);
                
            osDelay(50);
            
            lwip_send(fc->data_sock, temp, cb, 0);
            UART_Printf("ftpd: %s", temp);
        } while(ERR_OK == res);
        
        f_closedir(&dir);
    } else {
        UART_Printf("ftpd: f_opendir() failed. Error was %s (%d)\r\n", ff_errstr(res), res);
    }

    lwip_shutdown(fc->data_sock, SHUT_RDWR);
    lwip_close(fc->data_sock);
    fc->data_sock = -1;

    msg(fc, 226, "Transfer complete.");
    //msg(fc, 200, "489 bytes sent in 0.0032 seconds (148.17 Kbytes/s)");
}


extern struct netif gnetif;


static void 
cmd_pasv(ftp_client_t *fc, const char *args, uint16_t len)
{
    uint16_t port;
    char reply[64];
    struct sockaddr_in name;
    
    port = (uint16_t)(40000 + rand() % 25500);
    
    snprintf(reply, sizeof(reply), "Entering passive mode (%d,%d,%d,%d,%d,%d)",
        gnetif.ip_addr.addr & 0xFF,
        (gnetif.ip_addr.addr >> 8) & 0xFF,
        (gnetif.ip_addr.addr >> 16) & 0xFF,
        (gnetif.ip_addr.addr >> 24) & 0xFF,
        port & 0xFF,
        (port >> 8) & 0xFF
    );

    if(fc->pass_sock > 0) {
        lwip_shutdown(fc->pass_sock, SHUT_RDWR);
        lwip_close(fc->pass_sock);
    }
    
    fc->pass_sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fc->pass_sock < 0) {
        msg(fc, 451, "Cannot create socket");
        return;
    }
    
    name.sin_family = AF_INET;
    name.sin_len = sizeof(name);
    name.sin_port = port;
    name.sin_addr.s_addr = gnetif.ip_addr.addr;
    
    memset(&name.sin_zero, 0, sizeof(name.sin_zero));
    
    if(0 != lwip_bind(fc->pass_sock, (struct sockaddr *)&name, sizeof(name))) {
        msg(fc, 451, "Cannot bind to address on passive mode socket");
        lwip_close(fc->data_sock);
        fc->data_sock = -1;
        return;
    }
    
    if(0 != lwip_listen(fc->pass_sock, 4)) {
        msg(fc, 451, "Cannot listen on passive mode socket");
        lwip_close(fc->data_sock);
        fc->data_sock = -1;
        return;
    }
    
    msg(fc, 227, "%s", reply);
    fc->mode = FTP_MODE_PASSIVE;
}


static void 
cmd_pwd(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 257, "\"%s\"", fc->curdir);
}


static void
cmd_type(ftp_client_t *fc, const char *args, uint16_t len)
{
    msg(fc, 200, "Type set to %s", args);
}


static void 
cmd_put(ftp_client_t *fc, const char *args, uint16_t len)
{
    FRESULT err;
    char path[PATH_MAX];
    
    args += 2;

    set_drive(1);
    snprintf(path, sizeof(path), "1:/%s", args);

    UART_Printf("ftpd: PUT \"%s\"\r\n", path);
    msg(fc, 150, "Ok to send data");
    
    err = f_open(&fc->file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if(FR_OK == err) {
        fc->file_opened = 1;
        fc->fsize = 0;
    } else {
        UART_Printf("ftpd: Cannot open/access file. Error was %s (%d)\r\n", ff_errstr(err), err);
        msg(fc, 550, "Cannot open/access %\"s\": %s", args, ff_errstr(err));
    }
}


static void 
cmd_get(ftp_client_t *fc, const char *args, uint16_t len)
{
    FRESULT err;
    u32_t rd, total;
    FIL f;
    char path[PATH_MAX];
    
    args += 2;

    set_drive(1);
    snprintf(path, sizeof(path), "1:/%s", args);
    UART_Printf("ftpd: GET \"%s\"\r\n", path);
    
    err = f_open(&f, path, FA_READ);
    switch(err) {
        case FR_OK:
            /* Active mode
             */
            msg(fc, 150, "Opening BINARY mode data connection for %s", path);
            UART_Printf("ftpd: Sending in %s mode\r\n", fc->mode == FTP_MODE_ACTIVE? "ACTIVE": "PASSIVE");
            
            total = 0;
            while(!f_eof(&f)) {
                err = f_read(&f, _iobuf, sizeof(_iobuf), &rd);
                if(ERR_OK != err) {
                    UART_Printf("ftpd: Error reading file. Error was %s (%d)\r\n", ff_errstr(err), err);
                    break;
                }
                
                /* EOF? */
                if(0 == rd) {
                    break;
                }
                    
                UART_Printf("send %d\r\n", rd);
                lwip_send(fc->data_sock, _iobuf, rd, 0);
                total += rd;
                UART_Printf("total %d\r\n", total);
            }
            
            msg(fc, 226, "Transfer complete.");
            
            f_close(&f);
        
            lwip_shutdown(fc->data_sock, SHUT_RDWR);
            lwip_close(fc->data_sock);
            fc->data_sock = NULL;

            UART_Printf("ftpd: %u bytes transferred in %u seconds.\r\n", total, 60);
            break;
            
        case FR_NO_FILE:
            msg(fc, 550, "Not found");
            break;
            
        default:
            UART_Printf("ftpd: Cannot open file. %s\r\n", ff_errstr(err));
            break;
    }
}


/* NB! DELE command on the directory will not delete it, if
 * directory is not empty! 
 */
static void 
cmd_dele(ftp_client_t *fc, const char *args, uint16_t len)
{
    char path[PATH_MAX];
    FRESULT res;
    FILINFO fi;
    
    snprintf(path, sizeof(path), "1:/%s", args);
    
    res = f_stat(path, &fi);
    if(FR_OK != res) {
        msg(fc, 451, "FATFS error %s", ff_errstr(res));
        return;
    }
    
    UART_Printf("ftpd: About to delete %s \"%s\"\r\n", (fi.fattrib & AM_DIR) != 0? "directory": "file", path);
    
    if((fi.fattrib & AM_DIR) != 0) {
        res = f_rmdir(path);
    } else {
        res = f_unlink(path);
    }
    
    if(FR_OK == res) {
        msg(fc, 250, "Deleted \"%s\"", path);
    } else {
        UART_Printf("ftpd: FATFS error %s\r\n", ff_errstr(res));
        msg(fc, 550, "FATFS error %s", ff_errstr(res));
    }
}


static void 
cmd_port(ftp_client_t *fc, const char *args, uint16_t len)
{
    uint32_t h1, h2, h3, h4, p1, p2;
    struct sockaddr_in ip;
    
    if(6 != sscanf(args, "%u,%u,%u,%u,%u,%u", &h4, &h3, &h2, &h1, &p2, &p1)) {
        msg(fc, 501, "Syntax error in parameters or arguments");
        return;
    }

    ip.sin_family = AF_INET;
    ip.sin_len = sizeof(ip);
    ip.sin_addr.s_addr = (h1&0xFF)<<24 | (h2&0xFF)<<16 | (h3&0xFF)<<8 | (h4&0xFF);
    ip.sin_port = (p1&0xFF)<<8 | (p2&0xFF);
    
    UART_Printf("ftpd: Establshing connection to %s:%u\r\n", _inet_ntoa(&ip.sin_addr), ip.sin_port);
    
    fc->data_sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fc->data_sock < 0) {
        UART_Printf("ftpd: Error creating new netconn");
        kill_client(fc);
        return;
    }
    
    if(lwip_connect(fc->data_sock, (struct sockaddr *)&ip, sizeof(ip)) < 0) {
        UART_Printf("ftpd: Failed to connect to %s:%u\r\n", _inet_ntoa(&ip.sin_addr), ip.sin_port);
        kill_client(fc);
        return;
    }
    
    msg(fc, 200, "PORT command successful.");
    fc->mode = FTP_MODE_ACTIVE;
}


static FRESULT
_write(FIL *fp, const char *buf, uint32_t len)
{
    FRESULT res;
    uint32_t offs, cb, wrtn;
    
    for(offs=0; offs<len; offs+=_MAX_SS) {
        cb = (offs+_MAX_SS) > len? len-offs: _MAX_SS;
        res = f_write(fp, buf+offs, cb, &wrtn);
        if(FR_OK != res) {
            break;
        }
        /* Out of free space? */
        if(wrtn < cb) {
            break;
        }
    }
    
    return res;
}


int
ftp_client_serve(ftp_client_t *cli, char *buf, int size, int serve_what)
{
    FRESULT res;
    char err = 0;
    
    if(FTP_SERVE_CMD == serve_what) {
        exec_command(cli, buf, size);
    } else {
        if(cli->file_opened) {
#if 0 // Kept here for further debugging       
            UART_Printf("ftpd: writing %d bytes\r\n", size);
#endif            
            res = _write(&cli->file, buf, size);
            if(FR_OK != res) {
                UART_Printf("ftpd: Failed to write data to file: %s\r\n", ff_errstr(res));
                err = 1;
            }
            /* Adjust counters */
            cli->fsize += size;
            cli->total += size;
        } else {
            UART_Printf("ftpd: Error: File not opened.\r\n");
            err = 1;
        }


        if(err) {
//            UART_Printf("ftpd: Closing connection due to previous error(s)\r\n");
//            lwip_shutdown(cli->data_sock, SHUT_RDWR);
//            lwip_close(cli->data_sock);
//            cli->data_sock = -1;
        }
    }
    
    return 0;
}


void
ftp_serve()
{
    ftp_t *ftp = &_ftp;
    ftp_client_t *cli;
    fd_set rfds, wfds;
    int i, res, cli_sock, fdmax;
    struct sockaddr_in peer_addr;
    struct timeval tmo;
    socklen_t addrlen;
    
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    FD_SET(ftp->listen_sock, &rfds);
    
    fdmax = ftp->listen_sock;
    
    for(i=0; i<MAX_FTP_CLIENTS; ++i) {
        cli = &ftp->clients[i];
        if(!cli->free) {
            FD_SET(cli->cmd_sock, &rfds);
            if(cli->cmd_sock > fdmax) {
                fdmax = cli->cmd_sock;
            }

            if(cli->data_sock > 0) {
                FD_SET(cli->data_sock, &rfds);
                if(cli->data_sock > fdmax) {
                    fdmax = cli->data_sock;
                }
            }

            if(cli->pass_sock > 0) {
                FD_SET(cli->pass_sock, &rfds);
                if(cli->pass_sock > fdmax) {
                    fdmax = cli->pass_sock;
                }
            }
        }
    }
    
    tmo.tv_sec = 0;
    tmo.tv_usec = 100;
    
    res = lwip_select(fdmax+1, &rfds, &wfds, NULL, &tmo);
    if(res > 0) {
        if(FD_ISSET(ftp->listen_sock, &rfds)) {
            /* New client connected
             */
            addrlen = sizeof(peer_addr);
            cli_sock = lwip_accept(ftp->listen_sock, (struct sockaddr *)&peer_addr, &addrlen);
            if(cli_sock > 0) {
                UART_Printf("ftpd: New connection from %s:%u\r\n", _inet_ntoa(&peer_addr.sin_addr), ntohs(peer_addr.sin_port));
                cli = new_client(ftp);
                if(NULL == cli) {
                    UART_Printf("ftpd: out of RAM. Aborting client\r\n");
                    lwip_shutdown(cli_sock, SHUT_RDWR);
                    lwip_close(cli_sock);
                } else {
                    cli->cmd_sock = cli_sock;
                    cli->addr = peer_addr;
                    UART_Printf("ftpd: Printing banner\r\n");
                    print_banner(cli);
                }
            } else {
                UART_Printf("ftpd: Failed to accept new connection. errno %d\r\n", errno);
            }
         }
            
        for(i=0; i<MAX_FTP_CLIENTS; ++i) {
            cli = &ftp->clients[i];
            if(cli->free)
                continue;
                
            /* Check command socket for data presence
             */
            if(FD_ISSET(cli->cmd_sock, &rfds)) {
                /* Read data 
                 */
                res = lwip_read(cli->cmd_sock, _iobuf, sizeof(_iobuf));
                if(res == 0) {

                    UART_Printf("ftpd: Client %s disconnected\r\n", _inet_ntoa(&cli->addr.sin_addr));

                    /* Client disconnected. Gratefully close associated
                     * sockets and free memory
                     */
                    lwip_shutdown(cli->cmd_sock, SHUT_RDWR);
                    lwip_close(cli->cmd_sock);

                    if(cli->data_sock > 0) {
                        lwip_shutdown(cli->data_sock, SHUT_RDWR);
                        lwip_close(cli->data_sock);
                    }

                    if(cli->pass_sock > 0) {
                        lwip_shutdown(cli->pass_sock, SHUT_RDWR);
                        lwip_close(cli->pass_sock);
                    }

                    free_client(ftp, cli);
                    
                } else if(res > 0) {
                    ftp_client_serve(cli, _iobuf, res, FTP_SERVE_CMD);
                }
            }
            
            /* Check data socket for data presence
             */
            if(cli->data_sock >= 0) {
                if(FD_ISSET(cli->data_sock, &rfds)) {
                    res = lwip_read(cli->data_sock, _iobuf, sizeof(_iobuf));
                    if(res == 0) {
                        /* Client disconnected. Close data socket
                         */
                        lwip_shutdown(cli->data_sock, SHUT_RDWR);
                        lwip_close(cli->data_sock);
                        cli->data_sock = -1;
                        
                        /* Close file, if opened 
                         */
                        if(cli->file_opened) {
                            f_close(&cli->file);
                            memset(&cli->file, 0, sizeof(cli->file));
                            cli->file_opened = 0;
                        }
                        
                        int msecs = time(NULL) - cli->t_start;
                        msg(cli, 226, "Transfer complete. %u bytes transferred in %d seconds", cli->fsize, msecs/1000);
                    } else if(res > 0) {
                        ftp_client_serve(cli, _iobuf, res, FTP_SERVE_DATA);
                    }
                }
            }
            
            if(cli->pass_sock > 0 && FD_ISSET(cli->pass_sock, &rfds)) {
                /* Client connected to our listening passive mode socket
                 */
                addrlen = sizeof(peer_addr);
                cli->data_sock = lwip_accept(cli->pass_sock, (struct sockaddr *)&peer_addr, &addrlen);

                UART_Printf("ftpd: New data connection from %s:%d\r\n", _inet_ntoa(&peer_addr.sin_addr), peer_addr.sin_port);
                
                 /* Close listening passive mode socket. We don't need
                  * it anymore. Connection accepted and we waiting for STOR
                  * command
                  */
                lwip_shutdown(cli->pass_sock, SHUT_RDWR);
                lwip_close(cli->pass_sock);
                cli->pass_sock = -1;

                cli->t_start = time(NULL);
                //msg(cli, 150, "Ok to send data");
            }
        }
    } else if(res < 0) {
        UART_Printf("ftpd: select() failed (res=%d). Error was %d\r\n", res, errno);
    }
}
