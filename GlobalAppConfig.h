#pragma once


#include <QString>
#include <QVariant>


typedef struct _app_config {
    QString urlmap;
    QString predefs;
    QString tpl_path;  
}
AppConfig;


extern AppConfig globalConfig;


extern bool readGlobalConfig(const QString&);


//#endif