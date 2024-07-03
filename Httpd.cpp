#include "Httpd.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QStringList>


Httpd::Httpd() : QObject(), _addr(QHostAddress::Any), _port(DEFAULT_HTTPD_PORT), _srv(), _clients(), _vars()
{
    setupSignals();
}


Httpd::~Httpd()
{
    cleanupBeforeExit();
}


void
Httpd::readPredefVars(void)
{
    QDir dir;
    QFile pdfv("predefs.txt");
    QString s, key, val;
    int pos;

    s = dir.currentPath();
    if (pdfv.open(QIODevice::ReadOnly)) {
        QTextStream in(&pdfv);

        while (!in.atEnd()) {
            s = in.readLine().trimmed();
            if (0 == s.length()) {
                continue;
            }

            /* Strip possible comments
             */
            pos = s.indexOf(";");
            if (pos < 0) {
                pos = s.indexOf("#");
            }

            if (pos >= 0) {
                s = s.left(pos).trimmed();
            }

            /* Skip empty lines 
             */
            if (0 == s.length()) {
                continue;
            }

            /* The possible syntax of configuration line is
             *  key = value
             * or
             *  key (in such case assigned value is an empty string)
             */
            pos = s.indexOf("=");
            if (pos >= 0) {
                key = s.left(pos).trimmed();
                val = s.mid(pos + 1).trimmed();
            }
            else {
                key = s;
                val = "";
            }

            _vars[key] = val;
        }
    }
}


#ifdef _DEBUG
void
Httpd::printPredefVars(void)
{
    QString val;

    foreach(QString key, _vars.keys()) {
        qDebug() << key << "=" << _vars[key];
    }
}
#endif


void 
Httpd::cleanupBeforeExit(void)
{
    /* Close listening socket */
    _srv.close();

    /* Disconnect all clients, if any */
    for(HttpClient* cl : _clients) {
        cl->disconnect();
        delete cl;
    }

    /* Free resources */
    // YET DUMMY
}


void
Httpd::setupSignals(void)
{
    QObject::connect(&_srv, &QTcpServer::newConnection, this, &Httpd::newConnection);
}


void
Httpd::newConnection()
{
    QTcpSocket *sock = _srv.nextPendingConnection();
    HttpClient* cl;

    if (nullptr != sock) {
        qDebug() << QString("MicroHTTPD: New connection from %1").arg(sock->peerAddress().toString());

        cl = new HttpClient(sock);
        _clients.append(cl);
    }
}


bool
Httpd::start(void)
{
    qDebug() << "MicroHTTPD: Start";
    if (_srv.listen(_addr, _port)) {
        qDebug() << QString("MicroHTTPD: Listening for incoming connections on %1:%2").arg(_addr.toString()).arg(_port);

        readPredefVars();
#ifdef _DEBUG
        printPredefVars();
#endif
        return true;
    }


    return false;
}


bool
Httpd::stop(void)
{
    /* Disconnect all clients */
    /* Free allocated resources */
    return true;
}


void
Httpd::serve(void)
{
}


HttpClient::HttpClient(QTcpSocket* s) : QObject(), _sock(s) 
{
    setupSignals();
}


HttpClient::~HttpClient()
{
}


void
HttpClient::serve(void)
{
}


void
HttpClient::disconnect(void)
{
}


void
HttpClient::readyRead(void)
{
    QByteArray rq;

    rq = _sock->readAll();

    qDebug() << "Command:" << rq;

    parseCommandAndReply(rq);
}


void
HttpClient::setupSignals(void)
{
    QObject::connect(_sock, &QTcpSocket::readyRead, this, &HttpClient::readyRead);
}


void
HttpClient::parseCommandAndReply(QByteArray& cmd)
{
    QByteArrayList lines;
    QString resp;

    /* Supported HTTP Methods for now is only GET and POST 
     */
    lines = cmd.split('\n');
    if (lines[0].startsWith("GET")) {
        resp = QString(
            "HTTP/1.1 200 Ok\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n"
            "<html>\r\n"
            "<head>\r\n"
            "<title>Оно живое!</title>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "<h2>Оно наконец-то заработало!</h2>\r\n"
            "</body>\r\n"
            "</html>\r\n"
        );

        _sock->write(resp.toUtf8());

        qDebug() << "Response 200\r\n" << resp;

        _sock->close();
    }
}

