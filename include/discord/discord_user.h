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
#include "discord/fandom_filter_token.h"

#include "GlobalHeaders/SingletonHolder.h"

namespace interfaces{
class Users;
}


namespace discord{

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
    //void ToggleFandomIgnores(QSet<int>);
    void SetFandomFilter(int, bool displayCrossovers);
    void SetFandomFilter(FandomFilter );
    FandomFilter GetCurrentFandomFilter() const;
    void SetPositionsToIdsForCurrentPage(QHash<int, int>);

    FandomFilter GetCurrentIgnoredFandoms()  const;
    void SetIgnoredFandoms(FandomFilter);

    int GetFicIDFromPositionId(int) const;
    QSet<int> GetIgnoredFics()  const;
    void SetIgnoredFics(QSet<int>);

    void ResetFandomFilter();
    void ResetFandomIgnores();
    void ResetFicIgnores();

    QString FfnID()  const;
    QString UserName()  const;
    QString UserID()  const;
    bool ReadsSlash();
    bool HasActiveSet();
    void SetFicList(core::RecommendationListFicData);
    QSharedPointer<core::RecommendationListFicData> FicList();
    std::chrono::system_clock::time_point LastActive();


    bool isValid = false;
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
    std::chrono::system_clock::time_point lastActive;
    std::chrono::system_clock::time_point timeoutLimit;

    QSharedPointer<core::RecommendationListFicData> fics;
    QSet<int> ignoredFics;
    FandomFilter filteredFandoms;
    FandomFilter ignoredFandoms;
    QHash<int, int> positionToId;
    mutable QReadWriteLock lock;
};


struct Users{
    void AddUser(QSharedPointer<User>);
    bool HasUser(QString);
    QSharedPointer<User> GetUser(QString);
    bool LoadUser(QString user);
    void InitInterface(QSqlDatabase db);
    void ClearInactiveUsers();
    QHash<QString,QSharedPointer<User>> users;
    QReadWriteLock lock;

    QSharedPointer<interfaces::Users> userInterface;
};
}

BIND_TO_SELF_SINGLE(discord::Users);
