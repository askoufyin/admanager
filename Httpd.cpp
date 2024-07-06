#include "Httpd.h"
#include "GlobalAppConfig.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QRegularExpression>


Httpd::Httpd() : QObject(), _addr(QHostAddress::Any), _port(DEFAULT_HTTPD_PORT), _srv(), _clients(), _vars(), _urls()
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
            s = in.readLine().trimmed().toUtf8();
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


void
Httpd::readUrlMap(void)
{
    QFile urlMap = QFile(globalConfig.urlmap);
    QString map, url, arg;
    int cpos, ws;

    if (urlMap.open(QIODevice::ReadOnly)) {
        while (!urlMap.atEnd()) {
            map = urlMap.readLine();

            cpos = map.indexOf(';');
            if (cpos < 0) {
                cpos = map.indexOf('#');
            }

            /* cut off comments */
            if (cpos >= 0) {
                map = map.left(cpos);
            }

            map = map.trimmed();
            if (map.length() > 0) {
                ws = map.indexOf(' ');
                if (ws < 0) {
                    ws = map.indexOf('\t');
                }

                if (ws < 0) {
                    qDebug() << "urlmap:" << map;
                }
                else {
                    url = map.left(ws).trimmed();
                    arg = map.mid(ws + 1).trimmed();

                    qDebug() << "urlmap: " << url << "=" << arg;

                    _urls[url] = arg;
                }
            }
        }

        urlMap.close();
    }
}



#ifdef _DEBUG
void
Httpd::printPredefVars(void)
{
    QString val;

    foreach(QString key, _vars.keys()) {
        qDebug() << "var:" << key << "=" << _vars[key];
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
Httpd::setupSignals(void) const
{
    QObject::connect(&_srv, &QTcpServer::newConnection, this, &Httpd::newConnection);
}


void
Httpd::newConnection()
{
    QTcpSocket *sock = _srv.nextPendingConnection();
    HttpClient* cl;

    if (nullptr != sock) {
        qDebug() << QString("New connection from %1").arg(sock->peerAddress().toString());

        cl = new HttpClient(this, sock);

        QObject::connect(cl, SIGNAL(clientClosedConnection(HttpClient*)), this, SLOT(clientClosedConnection(HttpClient*)));

        _clients.append(cl);
    }
}


void
Httpd::clientClosedConnection(HttpClient* cl)
{
    int i = _clients.indexOf(cl);
    delete _clients[i];

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
        readUrlMap();

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


QString
Httpd::getVar(const QString& id, const QString& dflt) const
{
    if (_vars.contains(id)) {
        return _vars[id];
    }

    return dflt;
}


QString
HttpClient::getVar(const QString& id, const QString& dflt) const
{
    if (_server) {
        return _server->getVar(id, dflt);
    }

    return "";
}


HttpClient::HttpClient(Httpd *_srv, QTcpSocket* s, StringMap *map) : QObject(), 
    _server(_srv), _sock(s), _method(HttpMethod::Unsupported), _uri(), _proto(), _vars(map)
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
    QByteArray rq = _sock->readAll();
#if 0
    qDebug() << "Command:" << rq;
#endif
    parseCommandAndReply(rq);
}


void
HttpClient::setupSignals(void) const
{
    QObject::connect(_sock, &QTcpSocket::readyRead, this, &HttpClient::readyRead);
}


HttpMethod 
HttpClient::httpMethodId(const QString& line) const
{
    if (line.startsWith("GET")) {
        return HttpMethod::GET;
    }
    else if (line.startsWith("PUT")) {
        return HttpMethod::PUT;
    }
    else if (line.startsWith("POST")) {
        return HttpMethod::POST;
    }
    else if (line.startsWith("HEAD")) {
        return HttpMethod::HEAD;
    }

    return HttpMethod::Unsupported;
}


void 
HttpClient::parseHttpHeader(const QByteArrayList& hdr)
{
    QString line;
    QStringList cargs;
    int nline;

    nline = 0;
    foreach(line, hdr) {
        line = line.trimmed(); // get rid of \r\n

        if (0 == line.length()) {
            /* End of header, request body follows
            */
            qDebug() << "--- End of header, optional request content follows ---";
            break;
        }

        if (0 == nline) {
            cargs = line.split(" ");
            if (cargs.count() != 3) {
                qDebug() << "Misspelled header?" << line;
            }
            else {
                _method = httpMethodId(cargs[0]);
                switch (_method) {
                case HttpMethod::GET:
                    break;
                case HttpMethod::HEAD:
                    break;
                case HttpMethod::POST:
                    break;
                case HttpMethod::PUT:
                    break;
                default:
                    qDebug() << QString("Http method %1 unsupported yet").arg(cargs[0]); 
                    return;
                }
            }

            _uri = cargs[1].trimmed();
            _proto = cargs[2].trimmed();

            qDebug() << "URI:" << _uri;
            qDebug() << "Protocol Version:" << _proto;
        }
        else {
            cargs = line.split(": ");
            if (2 == cargs.count()) {
                //_header[cargs[0]] = cargs[1];
                qDebug() << cargs[0] << ":" << cargs[1];
            }
            else {
                qDebug() << "Error: " << line;
            }
        }
        ++nline;
    }
}


bool
HttpClient::validMacroName(const QByteArrayView& src, int i)
{
    QChar c;

    if (src.length() < 2) {
        return false;
    }

    c = src[i];
    if ('$' != c || '%' != c) {
        return false;
    }

    /* $$ and %% are valid predeined macroses, representing characters $ and % respectively
     */
    if (c == src[i+1]) {
        return true;
    }

    /* $macro, ${macro} or $(macro) is valid
     */
    c = src[i+1];
    if (c.isLetterOrNumber() || c == '_') {
        return true;
    }

    return false;
}


static inline bool _validChar(char c, bool spaces = false)
{
    if (spaces && (' ' == c || '\t' == c)) {
        return true;
    }
    return QChar::isLetter(c) || QChar::isDigit(c) || ('_' == c);
}


int
HttpClient::scanForMacroChar(const QByteArrayView& s) const {
    int i;

    for (i = 0; i < s.size(); ++i) {
        if (s[i] == '$' || s[i] == '%') {
            return i;
        }
    }

    return -1;
}


bool
HttpClient::haveMacro(const QByteArrayView& src, int *foundIndex, int *foundLen, QString& mname)
{
    int idx, len, maxsize;
    char c, temp[512];

    maxsize = src.length();

    idx = scanForMacroChar(src);
    if (idx < 0) {
        return false;
    }

    /* Handle special $$ and %% sequences. Replace it to &dollar; and &percent;
     */
    c = src[idx];
    if (idx < (src.length())) {
        char c1 = src[idx + 1];
        if (c == c1) {
            mname = QChar(c);
            *foundIndex = idx;
            *foundLen = 2;

            return true;
        }
    }

    /* Check for valid macro name. It must start from alphanumeric character or underscore 
     */
    c = src[idx+1]; // for debugging

    if ('(' == c || '{' == c) {
        /* Special case for macro names containing spaces: read all until eof or closing brace found
         */
        char closing = ('(' == c) ? ')' : '}';
        
        *foundIndex = idx++;

        /* len=3 takes in account $, ({ and })
         */
        c = src[++idx];
        for (len = 2; closing != src[idx] && _validChar(src[idx], true); ++idx, ++len) {
            temp[len - 2] = src[idx];
        }

        temp[len - 2] = 0;

        *foundLen = len;
        mname = temp;

        return true;
    }

    if (_validChar(c)) {
        char cc;

        *foundIndex = idx++;

        for (len = 0; _validChar(src[idx]); ++idx, ++len) {
            cc = src[idx];
            temp[len] = cc;
        }

        temp[len] = 0;

        *foundLen = len;
        mname = temp;

        return true;
    }

    return false;
}


QByteArray
HttpClient::expandVar(const QByteArray& src)
{
    return src;
}


QByteArray
HttpClient::expandTemplateVars(const QByteArray& src)
{
    QByteArray res;
    int foundIdx, foundLen;
    QString mname, mval;

    res = src;
    while (haveMacro(res, &foundIdx, &foundLen, mname)) {
        qDebug() << QString("Found macro: '%1' at %2(%3)").arg(mname).arg(foundIdx).arg(foundLen);

        /* Handle special cases: $$ and %% 
         */
        if (foundLen == 2) {
            switch (res[foundIdx]) {
            case '$':
                res = res = res.left(foundIdx) + "&dollar;" + res.mid(foundIdx + foundLen);
                break;
            case '%':
                res = res = res.left(foundIdx) + "&percnt;" + res.mid(foundIdx + foundLen);
                break;
            }
        }
        else {
            mval = _server->getVar(mname, "(undefined)");
            res = res.left(foundIdx) + mval.toUtf8() + res.mid(foundIdx + foundLen + 1) + "<br/>\n";
            qDebug() << res;
        }
    }

    return res;
}


void
HttpClient::parseCommandAndReply(QByteArray& cmd)
{
    QByteArrayList lines;
    QByteArray c;
    QString content, resp, uri;
    QFile ftmpl;
    int clen; // , errcode;

    /* Supported HTTP Methods for now is only GET and POST 
     */
    lines = cmd.split('\n');
    parseHttpHeader(lines);

    if (lines[0].startsWith("GET")) {
        uri = QDir::currentPath() + "\\" + globalConfig.tpl_path + "\\" + _server->mapUrl(_uri);

        uri = QDir::cleanPath(uri);

        qDebug() << _uri << " mapped to " << uri;

        ftmpl.setFileName(uri);

        if (!ftmpl.open(QIODevice::ReadOnly)) {
            content = QString(
                "<html>\r\n"
                "<head>\r\n"
                "<title>Внутренняя ошибка</title>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "<h2>Невозможно открыть файл %1 для чтения.</h2>\r\n"
                "</body>\r\n"
                "</html>\r\n"
            )
                .arg(ftmpl.fileName());

            c = content.toUtf8();
            clen = c.length();

            // Http error 500
            resp = QString("HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: %1; charset=%2\r\n"
                "Content-Length: %3\r\n"
                "\r\n")
                .arg("text/html")
                .arg("utf-8")
                .arg(clen);
        }
        else {
            content = ftmpl.readAll();

            c = expandTemplateVars(content.toUtf8());
            clen = c.length();

            resp = QString(
                "HTTP/1.1 200 Ok\r\n"
                "Content-Type: %1; charset=%2\r\n"
                "Content-Length: %3\r\n"
                "\r\n"
            )
                .arg("text/html")
                .arg("utf-8")
                .arg(clen);
        }

        
        resp = expandTemplateVars(resp.toUtf8());

        _sock->write(resp.toUtf8());
        _sock->write(c);

        //qDebug() << "Response 200\r\n" << resp;

        /* Если connection: keep-alive то сокет не закрывается! */
#if 0
        if ("close" == _header["Connection"]) {
            _sock->close();
            emit clientClosedConnection(this);
        }
        else if ("keep-alive" == _header["Connection"]) {
        }
#endif
    }
}


void
HttpClient::clientClosedConnection(HttpClient* client)
{
}
