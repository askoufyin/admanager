#pragma once


#include "Consts.h"


#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QTcpSocket>
#include <QSocketNotifier>
#include <QTcpServer>


/*! \brief HTTP protocol methods
 * See RFC9110 for detailed description of each method
 * GET and HEAD support mandatory, all other are optional
 */ 
enum class HttpMethod {
    Unsupported,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
};


typedef QMap<QString, QString> StringMap;


typedef struct _stringspan
{
    int start;
    int length;
}
stringspan_t;


class Httpd;


class HttpClient: public QObject {
protected:
    Httpd* _server;
    QTcpSocket *_sock;
    QString _uri;
    QString _proto;
    StringMap *_vars;// header;
    HttpMethod _method;
    bool _keep_alive;
protected:
    void setupSignals(void) const;
    HttpMethod httpMethodId(const QString&) const;
    void parseHttpHeader(const QByteArrayList&);
    int scanForMacroChar(const QByteArrayView&) const;
public:
    HttpClient(Httpd *, QTcpSocket*, StringMap * = nullptr);
    virtual ~HttpClient();
    virtual void serve(void);
    virtual void disconnect(void);
    virtual void parseCommandAndReply(QByteArray&);
    virtual bool haveMacro(const QByteArrayView&, int*, int*, QString&);
    virtual bool validMacroName(const QByteArrayView&, int);
    virtual QByteArray expandVar(const QByteArray&);
    virtual QByteArray expandTemplateVars(const QByteArray&);
    virtual QString getVar(const QString &, const QString& = "") const;
    //virtual int findMacro(const QByteArrayView&, int);
signals:
    void clientClosedConnection(HttpClient*);
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
    QMap<QString, QString> _vars;
    QMap<QString, QString> _urls;
protected:
    void setupSignals(void) const;
    void cleanupBeforeExit(void);
    void readUrlMap(void);
    void readPredefVars(void);
#ifdef _DEBUG
    void printPredefVars(void);
#endif
protected slots:
    void newConnection();
    void clientClosedConnection(HttpClient*);
public:
    Httpd();
    virtual ~Httpd();
    virtual bool start(void);
    virtual bool stop(void);
    virtual void serve(void);
    QString getVar(const QString&, const QString & = "") const;
public:
    QString mapUrl(const QString& url) {
        return _urls[url];
    }
};

