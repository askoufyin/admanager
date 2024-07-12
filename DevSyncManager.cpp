#include "DevSyncManager.h"


DeviceSyncManager::DeviceSyncManager():
    _state(NotConnected), _sock(nullptr)
{
}


DeviceSyncManager::~DeviceSyncManager()
{
}


void
DeviceSyncManager::queryPlaylist()
{

}


bool
DeviceSyncManager::connectToDevice(void)
{
    _sock = new QTcpSocket(this);

    
    return false;
}


void
DeviceSyncManager::disconnectFromDevice(void)
{
    if (DeviceSyncManager::Connected == _state) {
        // nothing yet
    }

    _state = NotConnected;
    
    if (nullptr != _sock) {
        delete _sock;
        _sock = nullptr;
    }
}


void
DeviceSyncManager::synchronize(void)
{
}

