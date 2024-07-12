#include "FileSyncer.h"


SingleFileTransfer::SingleFileTransfer() : 
    _sock(nullptr), _name(), _file(), _dir(TransferDir::NONE), _size(0), _current(0), _cksum(0)
{
}


SingleFileTransfer::~SingleFileTransfer()
{
    //[](const QString& t) { return t.trimmed(); };
}


/*! \brief Starts file transfer procedure
 * member _name must contain fully qualified file name
 */
bool
SingleFileTransfer::start(void)
{
    if (0 == _name.length()) {
        qDebug() << __FUNCTION__ ": No file name specified";
        emit error(this);
        return false;
    }

    if (!QFile::exists(_name)) {
        qDebug() << __FUNCTION__ ": file" << _name << "not found";
        emit error(this);
        return false;
    }

    _file.setFileName(_name);
    if (_file.open(QIODevice::ReadOnly)) {
        _size = _file.size();
        qDebug() << __FUNCTION__ ": starting transfer of" << _name << QString("(%1 bytes)").arg(_size);
    }
    else {
        qDebug() << __FUNCTION__ ": failed to open file. Is file exist?";
    }

    emit aboutToStart(this);
    return true;
}


bool
SingleFileTransfer::connectTo(const QString& host, quint16 port)
{
    if(nullptr == _sock) {
        _sock = new QTcpSocket();

        QObject::connect(_sock, &QTcpSocket::connected, this, &SingleFileTransfer::socketConnected);
        QObject::connect(_sock, &QTcpSocket::bytesWritten, this, &SingleFileTransfer::bytesWritten);

        _sock->connectToHost("localhost", 7001);

        return true;
    }

    return false;
}


void
SingleFileTransfer::socketConnected(void)
{
    _current = 0;
}


void
SingleFileTransfer::poll(void)
{
    if(nullptr == _sock) {
        return;
    }

    if(_sock->state() == QAbstractSocket::ConnectedState) {
#if 0
        if(_sock->bytesWritten() == 0) {

        }
#endif
    }
}


bool
SingleFileTransfer::pause(void)
{
    return true;
}


bool
SingleFileTransfer::stop(void)
{
    return true;
}


bool
SingleFileTransfer::transferBlock(void)
{
    return true;
}


void
SingleFileTransfer::bytesWritten(quint64 nb)
{
    _current += nb;
    qDebug() << __FUNCTION__ << nb << "bytes of" << _size << "written";
}


FileSyncer::FileSyncer() : _files(), _xfer(nullptr)
{
}


FileSyncer::~FileSyncer()
{
    if (nullptr != _xfer) {
        _xfer->abort();
        delete _xfer;
    }
}


void
FileSyncer::setupSignals(void)
{
}


bool
FileSyncer::queueFiles(const QStringList& _list)
{
    _files.append(_list);
    return true;
}


bool
FileSyncer::queueFile(const QString& _file)
{
    _files.append(_file);
    return true;
}


bool
FileSyncer::transferCurrentPart(void)
{
    return true;
}

