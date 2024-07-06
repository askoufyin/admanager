#include "Client.h"


Client::Client() : QObject(), _sock(), _state(PROTO_STATE_INITIAL)
{
    QObject::connect(&_sock, SIGNAL(connected()), this, SLOT(clientConnected()));
}


Client::~Client()
{
}


bool
Client::isAlive(void) const 
{
    return true;
}


void
Client::clientConnected(void)
{
    char temp[sizeof(msg_server_info_t) + sizeof(char) * 257];
    msg_server_info_t *msg = (msg_server_info_t *)temp;
    const char* sname = "Test server";

    _state = PROTO_STATE_HANDSHAKE;
    
    /* Send server info 
     */
    msg->version = PROTOCOL_VERSION;
    msg->flags = 0;
    strncpy_s(msg->name, sizeof(char) * 256, sname, strlen(sname));

    _sock.write(temp, sizeof(temp));
}

