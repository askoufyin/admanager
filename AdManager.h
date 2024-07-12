#pragma once


#include <QtWidgets/QMainWindow>
#include <QSystemTrayIcon>
#include <QTcpServer>
#include <QMenu>


#include "ui_AdManager.h"


#include "Httpd.h"
#include "UserDb.h"
#include "Client.h"
#include "FileSyncer.h"


class AdManager : public QMainWindow
{
    Q_OBJECT

public:
    AdManager(QWidget *parent = nullptr);
    virtual ~AdManager();
protected:
    int _win_on_start;
    Httpd *_httpd;
    QMenu* _traymenu;
    QSystemTrayIcon* _sicon;
    UsersDb _users;
    QTcpServer _cserv;          // TCP server for command port
    QTcpServer _dserv;          // TCP server for data transfer port
    ClientList _clients;
protected:
    void setupDefaults();
    void readConfig();
    void createTrayIcon();
    void createTrayMenu();
    void startHttpd();
    void stopHttpd();
    void startListeners();
    void showTrayMenu(QSystemTrayIcon::ActivationReason);
public slots:
    void showMainWindow(bool);
    void exitApp(bool);
    void actionSettings(bool);
    void actionPlaylistManage(bool);
    void updateDevices(bool);
    void clientConnected();
private:
    Ui::AdManagerClass ui;
};
