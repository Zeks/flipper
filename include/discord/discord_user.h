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

#include "GlobalHeaders/SingletonHolder.h"

namespace interfaces{
class Users;
}


namespace discord{
struct FandomFilterToken{
    FandomFilterToken(int id, bool includeCrossovers){
        this->id = id;
        this->includeCrossovers = includeCrossovers;
    }
    int id = -1;
    bool includeCrossovers = false;
};

struct FandomFilter{
    void RemoveFandom(int id){
        fandoms.remove(id);
        QList<FandomFilterToken> newTokens;
        for(auto token: tokens)
        {
            if(token.id != id)
                newTokens.push_back(token);
            tokens =newTokens;
        }
    }
    void AddFandom(int id, bool inludeCrossovers){
        fandoms.insert(id);
        tokens.push_back({id, inludeCrossovers});
    }

    QSet<int> fandoms;
    QList<FandomFilterToken> tokens;
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
    QHash<QString,QSharedPointer<User>> users;
    QReadWriteLock lock;

    QSharedPointer<interfaces::Users> userInterface;
};
}

BIND_TO_SELF_SINGLE(discord::Users);
