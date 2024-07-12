#pragma once


#include <QObject>
#include <QTcpSocket>


/*! /brief Synchronization manager for single remote client
 */
class DeviceSyncManager: public QObject
{
    Q_OBJECT
public:
    enum State {
        NotConnected = 0,
        Connecting,
        Connected,
        Failed
    };
protected:
    enum State _state;
    QTcpSocket* _sock;
public:
    DeviceSyncManager();
    virtual ~DeviceSyncManager();
    virtual bool connectToDevice(void);
    virtual void disconnectFromDevice(void);
    virtual void queryPlaylist(void);
    virtual void synchronize(void);
};


