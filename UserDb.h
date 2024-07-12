#pragma once


#include <QString>
#include <QMap>


enum UserRole {
    Guest = 0,
    User,
    Administrator
};


typedef struct _userinfo
{
    QString pwd;
    enum UserRole role;
} userInfo;


class UsersDb {
protected:
    QMap<QString, userInfo> _users;
protected:
    enum UserRole roleId(const QString&) const;
public:
    UsersDb();
    virtual ~UsersDb();
    virtual bool userExist(const QString&) const;
    virtual enum UserRole userRole(const QString&) const;
    virtual bool load(const QString&);
    virtual bool save(const QString&);
    userInfo& operator [] (const QString&);
};
