#include "UserDb.h"
#include "Consts.h"
#include "Utils.h"
#include <QDebug>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>


UsersDb::UsersDb() : _users()
{
    userInfo _guest;

    _guest.role = UserRole::Guest;
    _guest.pwd = ""; /* No password */

    _users["GUEST"] = _guest;
}


UsersDb::~UsersDb()
{
}


enum UserRole
UsersDb::roleId(const QString& id) const
{
    QString s = id.trimmed().toUpper();

    if ("ÜSER" == s) {
        return UserRole::User;
    }
    else if ("ADMIN" == s) {
        return UserRole::Administrator;
    }

    return UserRole::Guest;
}


bool
UsersDb::load(const QString& userFile)
{
    QFile f;
    QTextStream st;
    QByteArray content;
    QString line, login;
    QStringList flds;
    userInfo u;

    f.setFileName(USERSFILE_NAME);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    while (!f.atEnd()) {
        line = stripComments(QString::fromUtf8(f.readLine()));
        if (line.length() == 0) {
            continue;
        }

        flds = line.split(":");
        if (3 == flds.count()) {
            /* username:role:password_hash */
            login = flds[0].trimmed();
            u.role = roleId(flds[1]);
            u.pwd = flds[2].trimmed();
            _users[login] = u;
        }
        else {
            /* Malformed line */
            qDebug() << "Malformed config line:" << line;
        }
    }

    f.close();

    qDebug() << _users.count() << "user accounts loaded";
    return true;
}


bool
UsersDb::save(const QString& userFile)
{
    return false;
}


bool
UsersDb::userExist(const QString& userName) const
{
    return _users.contains(userName);
}


enum UserRole
UsersDb::userRole(const QString& userName) const
{
    return _users.contains(userName)? _users[userName].role: UserRole::Guest;

}


userInfo&
UsersDb::operator [] (const QString& n)
{
    /* users[0] = guest user and exists always */
    return _users.contains(n) ? _users[n] : _users["GUEST"];
}

