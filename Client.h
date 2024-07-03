#ifndef __CLIENT_CLASS_INCLUDED__
#define __CLIENT_CLASS_INCLUDED__


#include <QObject>
#include <QList>
#include <QTcpSocket>


#include "Consts.h"
#include "Protocol.h"


class Client: public QObject {
	Q_OBJECT
public:
	Client();
	virtual ~Client();
	virtual bool isAlive(void) const;
public slots:
	void clientConnected(void);
signals:
protected:
	QTcpSocket _sock;
	enum _proto_state _state;
};


typedef QList<Client> ClientList;


#endif
