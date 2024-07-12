#include <QObject>
#include <QDir>
#include <QMessageBox>
#include <QCryptographicHash>
#include "AdManager.h"
#include "GlobalAppConfig.h"
#include "DlgPlaylists.h"
#include "DlgSettings.h"
#include "DlgSyncProcess.h"
#include "Protocol.h"


AdManager::AdManager(QWidget* parent)
    : QMainWindow(parent), _users(), _clients(), _cserv()
{
    ui.setupUi(this);

    qDebug() << "----- Starting up -----";

    setupDefaults();
    readConfig();
    startHttpd();
    createTrayIcon();
    createTrayMenu();
    startListeners();
}


AdManager::~AdManager()
{
    delete _sicon;
}


void
AdManager::readConfig(void)
{
    qDebug() << "Reading global config";
    readGlobalConfig(QDir::currentPath());

    qDebug() << "Reading" << globalConfig.users;
    _users.load(USERSFILE_NAME);
    //
}


void
AdManager::createTrayIcon(void)
{
    _sicon = new QSystemTrayIcon(this);
    _sicon->setIcon(QIcon(":/icons/small/Folder.png"));
    _sicon->show();

    QObject::connect(_sicon, &QSystemTrayIcon::activated, this, &AdManager::showTrayMenu);
}


void
AdManager::startHttpd(void)
{
    _httpd = new Httpd();
    _httpd->start();
}


void
AdManager::stopHttpd(void)
{
    if (NULL != _httpd) {
        _httpd->stop();
        delete _httpd;
    }
}


void
AdManager::startListeners(void)
{
    qDebug() << QString("Starting listener on %1:%2")
        .arg(globalConfig.gl_address).arg(globalConfig.gl_port);

    QObject::connect(&_cserv, &QTcpServer::newConnection, this, &AdManager::clientConnected);

    qDebug() << QString("Starting data listener on %1:%2")
        .arg(globalConfig.gl_address).arg(globalConfig.dl_port);

    QObject::connect(&_cserv, &QTcpServer::newConnection, this, &AdManager::clientConnected);

    _cserv.listen(QHostAddress::Any, 7000);
    //_dataserv.listen(QHostAddress::Any, 7001);
}


void
AdManager::createTrayMenu(void)
{
    QAction* dflt;

    _traymenu = new QMenu(this);
    
    dflt = _traymenu->addAction(tr("Открыть"));
    _traymenu->setDefaultAction(dflt);
    _traymenu->addSeparator();
    _traymenu->addAction(ui.actDevPause);
    _traymenu->addAction(ui.actDevStart);
    _traymenu->addSeparator();
    _traymenu->addAction(ui.actSettings);

    connect(dflt, SIGNAL(triggered(bool)), this, SLOT(showMainWindow(bool)));
}


void
AdManager::showMainWindow(bool f)
{
    show();
}


void
AdManager::showTrayMenu(QSystemTrayIcon::ActivationReason reason)
{
    //switch (reason) {
    //case QSystemTrayIcon::DoubleClick:
    //    QMessageBox::information(nullptr, "Info", "Double click", QMessageBox::Ok);
    //    break;
    //default:
        _traymenu->exec(QCursor::pos());
    //    break;
    //}
}


void
AdManager::setupDefaults(void)
{
    this->ui.devList->setIconSize(QSize(32, 32));

    connect(this->ui.actExit, SIGNAL(triggered(bool)), this, SLOT(exitApp(bool)));
    connect(this->ui.actSettings, SIGNAL(triggered(bool)), this, SLOT(actionSettings(bool)));
    connect(this->ui.actPlayListManage, SIGNAL(triggered(bool)), this, SLOT(actionPlaylistManage(bool)));
    connect(this->ui.actUpdateAll, SIGNAL(triggered(bool)), this, SLOT(updateDevices(bool)));
    connect(&this->_cserv, SIGNAL(newConnection()), this, SLOT(clientConnected()));
}


void
AdManager::exitApp(bool f)
{
    (void)(f);

    QApplication::exit();
}


void
AdManager::actionPlaylistManage(bool f)
{
    (void)(f);

    DlgPlaylists dlg;

    dlg.exec();
}


void
AdManager::actionSettings(bool f)
{
    (void)(f);

    DlgSettings dlg;

    dlg.exec();
}


void
AdManager::updateDevices(bool f)
{
    (void)(f);

    DlgSyncProcess dlg;

    dlg.exec();
}


void
AdManager::clientConnected(void)
{
    Client* cli;

    if (_cserv.hasPendingConnections()) {
        cli = new Client(_cserv.nextPendingConnection());
        _clients.append(cli);
        cli->startSession();
    }
}


//void
//AdManager::gotUserName(void)
//{
//}
