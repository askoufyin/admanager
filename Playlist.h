#ifndef __PLAYLIST_H_INCLUDED__
#define __PLAYLIST_H_INCLUDED__


#include <QString>
#include <QStringList>


class Playlist {
protected:
    QString _name;
    QStringList _items;
public:
    Playlist() : _name(), _items() {}
    virtual ~Playlist() {}
    virtual bool saveToFile(const QString&) { 
        return false; 
    }
    virtual bool loadFromFile(const QString&) {
        return false;
    }
    virtual const QString name() const {
        return _name;
    }
    virtual void setName(const QString& newname) {
        _name = newname;
    }
    virtual QStringList& items() {
        return _items;
    }
};


#endif

