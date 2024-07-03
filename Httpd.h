#pragma once


#include "Consts.h"


#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QTcpSocket>
#include <QSocketNotifier>
#include <QTcpServer>


class HttpClient: public QObject {
protected:
    QTcpSocket *_sock;
protected:
    void setupSignals(void);
public:
    HttpClient(QTcpSocket* s);
    virtual ~HttpClient();
    virtual void serve(void);
    virtual void disconnect(void);
    virtual void parseCommandAndReply(QByteArray&);
protected slots:
    void readyRead(void);
};


typedef QList<HttpClient*> HttpClients;


class Httpd: public QObject {
public:
    enum {
        HTTP_OK = 200,
        HTTP_FORBIDDEN = 500,
    } ErrorCode;
protected:
    QHostAddress _addr;
    uint16_t _port;
    QTcpServer _srv;
    HttpClients _clients;
    QMap< QString, QString> _vars;
protected:
    void setupSignals(void);
    void cleanupBeforeExit(void);
    void readPredefVars(void);
#ifdef _DEBUG
    void printPredefVars(void);
#endif
protected slots:
    void newConnection();
public:
    Httpd();
    virtual ~Httpd();
    virtual bool start(void);
    virtual bool stop(void);
    virtual void serve(void);
};

