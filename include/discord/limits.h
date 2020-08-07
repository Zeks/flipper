#pragma once
#include <string>
#include <QString>
#include <QQueue>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QSqlDatabase>
#include <QWriteLocker>
#include <QHash>
#include <chrono>
#include "core/section.h"
//#include "Interfaces/discord/users.h"

namespace interfaces{
class Users;
}

namespace discord{
struct IgnoreFandom{
   int id;
    bool withCrosses;
};
enum EListType{
    elt_favourites = 0,
    elt_urls = 1
};
struct User{
    User(){InitFicsPtr();}
    User(QString userID, QString ffnID, QString name);
    User(const User &user);
    void InitFicsPtr();
    User& operator=(const User &user);
    int secsSinceLastsRecQuery();
    int secsSinceLastsEasyQuery();
    int secsSinceLastsQuery();
    void initNewRecsQuery();
    void initNewEasyQuery();
    int CurrentPage() const;
    void SetPage(int newPage);
    void AdvancePage(int value);
    bool HasUnfinishedRecRequest() const;
    void SetPerformingRecRequest(bool);
    void SetCurrentListId(int listId);
    void SetBanned(bool banned);
    void SetFfnID(QString id);
    void SetUserID(QString id);
    void SetUserName(QString name);

    QString FfnID();
    QString UserName();
    QString UserID();
    bool ReadsSlash();
    bool HasActiveSet();
    void SetFicList(core::RecommendationListFicData);
    QSharedPointer<core::RecommendationListFicData> FicList();

private:

    QString userID;
    QString userName;
    QString ffnID;
    int page = 0;
    int listId= 0;
    bool banned = false;
    bool readsSlash = false;
    bool hasUnfinishedRecRequest = false;

    std::chrono::system_clock::time_point lastRecsQuery;
    std::chrono::system_clock::time_point lastEasyQuery;

    QSharedPointer<core::RecommendationListFicData> fics;
    QSet<int> ignoredFandoms;
    int filteredFandom = -1;
    mutable QReadWriteLock lock;
};


struct Users{
    void AddUser(QSharedPointer<User>);
    bool HasUser(QString);
    QSharedPointer<User> GetUser(QString);
    bool LoadUser(QString user);
    void InitInterface(QSqlDatabase db);
    QHash<QString,QSharedPointer<User>> users;
    QReadWriteLock lock;

    QSharedPointer<interfaces::Users> userInterface;
};
}
