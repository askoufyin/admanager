#ifndef __PROTOCOL_H_INCLUDED__
#define __PROTOCOL_H_INCLUDED__


#include <stdint.h>


enum _protocol_msg_id {
    PROTO_MSG_HELLO = 100,          // С->К Проверка поддержки протокола
    PROTO_MSG_PLEASE_INTRODUCE,     // С->К Сервер просит представиться
    PROTO_MSG_LOGIN,                // К->C Логин
    PROTO_MSG_AUTH_REQ,             // C->К Требование авторизации
    PROTO_MSG_PASSWORD_REQUIRED,    // С->К Требуется предоставить пароль
    PROTO_MSG_PASSWORD,             // К->C Пароль
    PROTO_MSG_ME,                   // К->С Описание клиента
    PROTO_MSG_PROCEED,              // С->К Всё ок, сервер готов принимать команды
    PROTO_MSG_AUTH = 150,           // К->C Данные для авторизации
    PROTO_MSG_QUERY_PLAYLIST,       // К->С Запрос своего плейлиста
};


enum _proto_cmd {
    PROTO_CMD_PING = 0,				// Используется для определения времени последней активности

    PROTO_CMD_SERVER_INFO,		    // C->К первое сообщение после принятия соединения
    PROTO_CMD_CLIENT_INFO,		    // К->С в ответ на PROTO_CMD_SERVER_INFO
    PROTO_CMD_AUTH,					// Данные авторизации от клиента

    PROTO_CMD_CONTENT_INFO,
    PROTO_CMD_CONTENT_DATA,

    PROTO_CMD_QUERY_PLAYLIST,       // Запрос клиента к серверу о своём плейлисте
    PROTO_CMD_PLAYLIST,             // Ответ сервера клиенту с содержанием плейлиста

    PROTO_CMD_LAST
};


enum _proto_state {
    PROTO_STATE_INITIAL = 0,        // Начальная стадия
    PROTO_STATE_HANDSHAKE,          //
    PROTO_STATE_AUTH_FAIL,          // Авторизация не пройдена
};


#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable:4200)


typedef struct _msg_hdr
{
    uint8_t cmd;                    // Код команды PROTO_CMD_xxx
    uint16_t clen;                  // Длина данных команды
}
msg_hdr_t;


typedef struct _pmsg_sinfo
{
    uint16_t version;				// Версия сервера
    uint16_t flags;					// Битовые флаги SIF_xxx
    char name[];					// Имя сервера. Длина до конца пакета
}
msg_server_info_t;


typedef struct _pmsg_cinfo
{
    uint16_t version;               // Версия клиента
    uint8_t loclen;                 // Длина строки места расположения
    uint8_t location[];             // Физическое место расположения оборудования
}
msg_client_info_t;


#define SIF_OFFLINE     0x0001      // Сервер оффлайн и не принимает соединения
#define SIF_AUTHREQ		0x0002		// Требуется авторизация
#define SIF_AUTHORIZED  0x0004      // Авторизация пройдена успешно
#define SIF_AUTHFAILED  0x0008      // Авторизация провалена


#pragma warning(pop)
#pragma pack(pop)


#endif
