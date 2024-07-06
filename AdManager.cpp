#include <QObject>
#include <QDir>
#include "AdManager.h"
#include "GlobalAppConfig.h"
#include "DlgPlaylists.h"
#include "DlgSettings.h"


AdManager::AdManager(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    qDebug() << "----- Starting up -----";

    setupDefaults();
    readConfig();
    startHttpd();
    createTrayIcon();
    createTrayMenu();
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
}


void
AdManager::createTrayIcon(void)
{
    _sicon = new QSystemTrayIcon(this);
    _sicon->setIcon(QIcon(":/icons/Folder.png"));
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
AdManager::createTrayMenu(void)
{
    QAction* dflt;

    _traymenu = new QMenu(this);
    
    dflt = _traymenu->addAction(tr("Открыть"));
    _traymenu->setDefaultAction(dflt);
    _traymenu->addSeparator();
    _traymenu->addAction(ui.actDevPause);
    _traymenu->addAction(ui.actDevStart);
}


void
AdManager::showTrayMenu(void)
{
    _traymenu->exec(QCursor::pos());
}



void
AdManager::setupDefaults(void)
{
    this->ui.devList->setIconSize(QSize(32, 32));

    connect(this->ui.actExit, SIGNAL(triggered(bool)), this, SLOT(exitApp(bool)));
    connect(this->ui.actSettings, SIGNAL(triggered(bool)), this, SLOT(actionSettings(bool)));
    connect(this->ui.actPlayListManage, SIGNAL(triggered(bool)), this, SLOT(actionPlaylistManage(bool)));
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