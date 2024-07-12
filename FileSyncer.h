#pragma once


#include <QObject>
#include <QString>
#include <QQueue>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QFile>


#define DEFAULT_BLOCK_SIZE      8192


enum class TransferDir {
    NONE = 0,
    SEND,
    RECEIVE
};


class SingleFileTransfer :public QObject
{
    Q_OBJECT
protected:
    QTcpSocket* _sock;
    QString _name;
    QFile _file;
    TransferDir _dir;
    quint64 _size;
    quint64 _current;
    quint64 _cksum;
public:
    SingleFileTransfer();
    virtual ~SingleFileTransfer();
    virtual void setName(const QString& name) {
        _name = name;
    }
    virtual bool start(void);
    virtual bool pause(void);
    virtual bool stop(void);

    virtual bool transferBlock(void);
    virtual quint64 total(void) const {
        return _size;
    }
    virtual quint64 soFar(void) const {
        return _current;
    }
    virtual bool inProgress(void) const {
        return ((nullptr != _sock) && _sock->state() == QTcpSocket::ConnectedState) 
            && (_size != 0);
    }
    virtual bool isCompleted(void) const {
        return false;
    }
    virtual void abort(void) {
    }
    virtual void setSocket(QTcpSocket* s) {
        _sock = s;
    }
    virtual QTcpSocket* socket(void) const {
        return _sock;
    }
    virtual const QString& fileName(void) const {
        return _name;
    }
    virtual void poll(void);
protected:
    bool connectTo(const QString&, quint16);
    void socketConnected(void);
protected slots:
    void bytesWritten(quint64);
signals:
    void aboutToStart(SingleFileTransfer*);
    void error(SingleFileTransfer*);
    void complete(SingleFileTransfer*);
    void progress(SingleFileTransfer*);
};


class FileSyncer : public QObject
{
    Q_OBJECT
protected:
    QQueue<QString> _files;
    SingleFileTransfer* _xfer;
protected:
    void setupSignals(void);
public:
    FileSyncer();
    virtual ~FileSyncer();
    virtual bool queueFile(const QString&);
    virtual bool queueFiles(const QStringList&);
    virtual bool hasPendingFiles() const {
        return _files.length() > 0;
    }

    virtual void abortAll(void) {
        if (nullptr != _xfer) {
            _xfer->abort();
            delete _xfer;
            _xfer = nullptr;
        }
        _files.clear();
    }

    bool transferCurrentPart(void);

    virtual quint32 pendingCount(void) const {
        return _files.count() + (nullptr != _xfer? 1: 0);
    }

    virtual const QString currentName() const {
        if (nullptr != _xfer) {
            return _xfer->fileName();
        }
        return "";
    }

signals:
    void aboutToBegin(void);
    void nextFile(int, int);
    void transferError(void);
    void filePartSent(quint64, quint64);
public slots:

};
