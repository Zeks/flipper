#pragma once
#include <string>
#include <QString>
#include <QQueue>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QSqlDatabase>
#include <QWriteLocker>
#include <QHash>
#include <QUuid>
#include <chrono>
#include "core/section.h"
#include "discord/fandom_filter_token.h"
#include "discord/command_types.h"

#include "GlobalHeaders/SingletonHolder.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop


namespace interfaces{
class Users;
}


namespace discord{

struct WordcountFilter{
    uint64_t firstLimit = 0;
    uint64_t secondLimit = 0;
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
    void SetPerfectRngFics(QSet<int>);
    void SetGoodRngFics(QSet<int>);
    void SetPerfectRngScoreCutoff(int);
    void SetGoodRngScoreCutoff(int);
    void SetUserID(QString id);
    void SetUserName(QString name);
    void SetUuid(QString);
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

    QSet<int> GetPerfectRngFics();
    QSet<int> GetGoodRngFics();
    int GetPerfectRngScoreCutoff() const;
    int GetGoodRngScoreCutoff() const;

    void ResetFandomFilter();
    void ResetFandomIgnores();
    void ResetFicIgnores();

    QString FfnID()  const;
    QString UserName()  const;
    QString UserID()  const;
    QString GetUuid()  const;
    bool ReadsSlash();
    bool HasActiveSet();
    void SetFicList(QSharedPointer<core::RecommendationListFicData>);
    QSharedPointer<core::RecommendationListFicData> FicList();
    std::chrono::system_clock::time_point LastActive();

    int GetForcedMinMatch() const;
    void SetForcedMinMatch(int value);

    int GetForcedRatio() const;
    void SetForcedRatio(int value);

    bool GetUseLikedAuthorsOnly() const;
    void SetUseLikedAuthorsOnly(bool value);

    bool GetSortFreshFirst() const;
    void SetSortFreshFirst(bool value);

    bool GetStrictFreshSort() const;
    void SetStrictFreshSort(bool value);


    bool GetShowCompleteOnly() const;
    void SetShowCompleteOnly(bool value);

    bool GetHideDead() const;
    void SetHideDead(bool value);



    QString GetLastUsedRoll() const;
    void SetLastUsedRoll(const QString &value);

    SleepyDiscord::Snowflake<SleepyDiscord::Message> GetLastPageMessage() const;
    void SetLastPageMessage(const SleepyDiscord::Snowflake<SleepyDiscord::Message> &value);

    ECommandType GetLastPageType() const;
    void SetLastPageType(const ECommandType &value);

    int GetSimilarFicsId() const;
    void SetSimilarFicsId(int value);

    bool isValid = false;

    bool GetRngBustScheduled() const;
    void SetRngBustScheduled(bool value);


public:
    WordcountFilter GetWordcountFilter() const;
    void SetWordcountFilter(const WordcountFilter &value);

private:
    QString userID;
    QString userName;
    QUuid uuid;
    QString ffnID;
    int page = 0;
    int listId= 0;
    bool banned = false;
    bool readsSlash = false;
    bool hasUnfinishedRecRequest = false;
    bool useLikedAuthorsOnly = false;
    bool sortFreshFirst = false;
    bool showCompleteOnly = false;
    bool hideDead = false;
    bool strictFreshSort = false;
    bool rngBustScheduled = false;
    int similarFicsId = 0;
    int forcedMinMatch = 0;
    int forcedRatio = 0;

    QString lastUsedRoll = "all";

    std::chrono::system_clock::time_point lastRecsQuery;
    std::chrono::system_clock::time_point lastEasyQuery;
    std::chrono::system_clock::time_point lastActive;
    std::chrono::system_clock::time_point timeoutLimit;

    QSharedPointer<core::RecommendationListFicData> fics;
    QSet<int> perfectRngFics;
    QSet<int> goodRngFics;
    int perfectRngScoreCutoff;
    int goodRngScoreCutoff;
    QSet<int> ignoredFics;
    FandomFilter filteredFandoms;
    WordcountFilter wordcountFilter;
    FandomFilter ignoredFandoms;
    QHash<int, int> positionToId;
    ECommandType lastPageType = ct_display_page;
    SleepyDiscord::Snowflake<SleepyDiscord::Message> lastPageMessage;
    mutable QReadWriteLock lock;
};


struct Users{
    void AddUser(QSharedPointer<User>);
    void RemoveUserData(QSharedPointer<User>);
    bool HasUser(QString);
    QSharedPointer<User> GetUser(QString);
    bool LoadUser(QString user);
    void ClearInactiveUsers();
    QHash<QString,QSharedPointer<User>> users;
    QReadWriteLock lock;

    //QSharedPointer<interfaces::Users> userInterface;
};
}

BIND_TO_SELF_SINGLE(discord::Users);
