#include "Client.h"
#include "Protocol.h"


Client::Client(QTcpSocket *s) : QObject(), _sock(s), _state(PROTO_STATE_INITIAL), _sync(nullptr)
{
    /* Set up command socket */
    QObject::connect(_sock, SIGNAL(readyRead()), this, SLOT(readyRead()));

    _sync = new FileSyncer();
}


Client::~Client()
{
    if (nullptr != _sync) {
        _sync->abortAll();
        delete _sync;
    }
}


bool
Client::isAlive(void) const 
{
    return true;
}


void
Client::startSession()
{
    msg(PROTO_MSG_HELLO, "Hello!");
    msg(PROTO_MSG_AUTH_REQ, "This server prevents anonymous logins");
}


void
Client::readyRead()
{
    QByteArray line;
    QByteArray aarg;
    int i, acode;

    /* Data arrived at socket */
    line = _sock->readAll();
    qDebug() << "Client got" << line;

    i = line.indexOf(' ');
    if (i > 0) {
        acode = line.left(i).trimmed().toInt();
        aarg = line.mid(i + 1).trimmed();

        qDebug() << acode << aarg;
        
        switch (acode) {
        case PROTO_MSG_LOGIN:
            qDebug() << QString("Trying to log in as '%1'").arg(aarg);
            msg(PROTO_MSG_PASSWORD_REQUIRED, QString("Password required for '%1'").arg(aarg));
            break;

        case PROTO_MSG_PASSWORD:
            qDebug() << QString("Password hash supplied '%1'").arg(aarg);
            msg(PROTO_MSG_PROCEED, "User ok, you can proceed");
            break;
        }
    }

}


void
Client::msg(int code, const QString& text)
{
    QString data = QString("%1 %2\r\n").arg(code).arg(text);
    qDebug() << "Server sent" << data;
    _sock->write(data.toUtf8());
    _sock->flush();
}

