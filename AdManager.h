#pragma once


#include <QtWidgets/QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>


#include "ui_AdManager.h"


#include "Httpd.h"


class AdManager : public QMainWindow
{
    Q_OBJECT

public:
    AdManager(QWidget *parent = nullptr);
    virtual ~AdManager();
protected:
    Httpd *_httpd;
    QMenu* _traymenu;
    QSystemTrayIcon* _sicon;
protected:
    void setupDefaults();
    void createTrayIcon();
    void createTrayMenu();
    void startHttpd();
    void stopHttpd();
    void showTrayMenu();
public slots:
    void exitApp(bool);
    void actionSettings(bool);
    void actionPlaylistManage(bool);
private:
    Ui::AdManagerClass ui;
};
