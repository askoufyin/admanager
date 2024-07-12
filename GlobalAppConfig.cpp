#include "GlobalAppConfig.h"
#include <QSettings>


AppConfig globalConfig;


const char globalConfigFile[] = "AdManager.ini";


bool
readGlobalConfig(const QString& appPath)
{
    QSettings ini(globalConfigFile, QSettings::NativeFormat);

    globalConfig.windowOnStart = ini.value("windowOnStart", "3").toInt();
    globalConfig.urlmap = ini.value("urlMap", "conf\\urlmap.txt").toString();
    globalConfig.predefs = ini.value("predefs", "conf\\predefs.txt").toString();
    globalConfig.users = ini.value("users", "conf\\users.txt").toString();
    globalConfig.tpl_path = ini.value("templates", ".\\html").toString();
    globalConfig.gl_address = ini.value("listenAddr", "0.0.0.0").toString();
    globalConfig.gl_port = ini.value("listenport", "7000").toInt();
    globalConfig.dl_port = ini.value("dataport", "7001").toInt();

    return true;
}
