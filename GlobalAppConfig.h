#pragma once


#include <QString>
#include <QVariant>


enum {
    Start_Window_Normal = 0,
    Start_Window_Maximized,
    Start_Window_Minimized,
    Start_Window_Hidden,
};


typedef struct _app_config {
    int windowOnStart;
    QString urlmap;
    QString predefs;
    QString users;
    QString tpl_path;
    QString gl_address;
    quint16 gl_port;        // command listener port
    quint16 dl_port;        // data listener port
}
AppConfig;


extern AppConfig globalConfig;


extern bool readGlobalConfig(const QString&);


//#endif