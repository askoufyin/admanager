#include "GlobalAppConfig.h"
#include <QSettings>


AppConfig globalConfig;


const char globalConfigFile[] = "AdManager.ini";


bool
readGlobalConfig(const QString& appPath)
{
    QSettings ini(globalConfigFile, QSettings::NativeFormat);

    globalConfig.urlmap = ini.value("urlMap", "urlmap.txt").toString();
    globalConfig.predefs = ini.value("urlMap", "predefs.txt").toString();
    globalConfig.tpl_path = ini.value("templatesPath", ".\\html").toString();
    
    return true;
}




