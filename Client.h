#ifndef __CLIENT_CLASS_INCLUDED__
#define __CLIENT_CLASS_INCLUDED__


//#include <varargs.h>


#include <QObject>
#include <QList>
#include <QTcpSocket>


#include "Consts.h"
#include "Protocol.h"
#include "FileSyncer.h"


class Client: public QObject {
	Q_OBJECT
protected:
	QTcpSocket* _sock;
	enum _proto_state _state;
	FileSyncer* _sync;
public:
	Client(QTcpSocket *);
	virtual ~Client();
	virtual bool isAlive(void) const;
	virtual QTcpSocket* socket(void) {
		return _sock;
	}
	virtual void msg(int, const QString&);
	virtual void startSession();
public slots:
	virtual void readyRead();
};


typedef QList<Client *> ClientList;


#endif
