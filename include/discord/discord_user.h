/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include <string>
#include <QString>
#include <QQueue>
#include <QReadWriteLock>
#include <QReadLocker>
#include "sql_abstractions/sql_database.h"
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
    enum EFilterMode{
        fm_none = 0,
        fm_less = 1,
        fm_more = 2,
        fm_between = 3,
    };
    uint64_t firstLimit = 0;
    uint64_t secondLimit = 0;
    EFilterMode filterMode = fm_none;
};
struct LastPageCommandMemo{
    SleepyDiscord::Snowflake<SleepyDiscord::Message> message;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> channel;
};

struct LargeListToken{
    int counter = 0;
    QDate date;
};

struct User{
    User():lock(QReadWriteLock::Recursive){InitFicsPtr();}
    ~User() = default;
    User(QString userID, QString ffnID, QString name);
    User(QString userID, QString ffnID, QString name, QString uuid);
    User(const User &user);
    void InitFicsPtr();
    User& operator=(const User &user);
    int secsSinceLastsRecQuery();
    int secsSinceLastsEasyQuery();
    int secsSinceLastsQuery();
    void initNewRecsQuery();
    void initNewEasyQuery();
    int CurrentRecommendationsPage() const;
    void SetPage(int newPage);
    void AdvancePage(int value, bool countAsQuery = true);
    bool HasUnfinishedRecRequest() const;
    void SetPerformingRecRequest(bool);
    void SetCurrentListId(int listId);
    void SetBanned(bool banned);
    void SetFfnID(QString id);
    void SetPerfectRngFics(const QSet<int>&);
    void SetGoodRngFics(const QSet<int>&);
    void SetPerfectRngScoreCutoff(int);
    void SetGoodRngScoreCutoff(int);
    void SetUserID(QString id);
    void SetUserName(QString name);
    void SetUuid(QString);
    //void ToggleFandomIgnores(QSet<int>);
    void SetFandomFilter(int, bool displayCrossovers);
    void SetFandomFilter(const FandomFilter & );
    FandomFilter GetCurrentFandomFilter() const;
    void SetPositionsToIdsForCurrentPage(const QHash<int, int> &);

    FandomFilter GetCurrentIgnoredFandoms()  const;
    void SetIgnoredFandoms(const FandomFilter &);

    int GetFicIDFromPositionId(int) const;
    QSet<int> GetIgnoredFics()  const;
    void SetIgnoredFics(const QSet<int> &);

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

    LastPageCommandMemo GetLastPageMessage() const;
    void SetLastPageMessage(const LastPageCommandMemo &value);

    ECommandType GetLastPageType() const;
    void SetLastPageType(const ECommandType &value);

    int GetSimilarFicsId() const;
    void SetSimilarFicsId(int value);


    bool GetRngBustScheduled() const;
    void SetRngBustScheduled(bool value);

    WordcountFilter GetWordcountFilter() const;
    void SetWordcountFilter(WordcountFilter value);

    int GetDeadFicDaysRange() const;
    void SetDeadFicDaysRange(int value);


    int GetCurrentHelpPage() const;
    void SetCurrentHelpPage(int value);

    bool GetIsValid() const;
    void SetIsValid(bool value);
    int GetFavouritesSize() const;
    void SetFavouritesSize(int value);
    QDate GetLastLargeListRegenerationDate() const;
    void SetLastLargeListRegenerationDate(const QDate &value);
    int GetLargeListCounter() const;
    void SetLargeListCounter(int value);
    LargeListToken GetLargeListToken() const;
    void SetLargeListToken(const LargeListToken &value);

    int GetPerfectRngFicsSize() const;
    void SetPerfectRngFicsSize(int value);

    int GetGoodRngFicsSize() const;
    void SetGoodRngFicsSize(int value);

    bool GetTimeoutWarningShown() const;
    void SetTimeoutWarningShown(bool value);

    bool GetShownBannedMessage() const;
    void SetShownBannedMessage(bool value);

    bool GetBanned() const;

    bool GetSortGemsFirst() const;
    void SetSortGemsFirst(bool value);

    QString GetImpersonatedId() const;
    void SetImpersonatedId(const QString &value);

private:
    bool isValid = false;
    QString userID;
    QString impersonatedId;
    QString userName;
    QUuid uuid;
    QString ffnID;

    bool banned = false;
    bool shownBannedMessage = false;
    bool readsSlash = false;
    bool hasUnfinishedRecRequest = false;
    bool useLikedAuthorsOnly = false;
    bool sortFreshFirst = false;
    bool sortGemsFirst = false;
    bool showCompleteOnly = false;
    bool hideDead = false;
    bool strictFreshSort = false;
    bool rngBustScheduled = false;
    bool timeoutWarningShown = false;

    int currentRecsPage = 0;
    int currentHelpPage = 0;
    int listId= 0;
    int deadFicDaysRange = 365;
    int similarFicsId = 0;
    int forcedMinMatch = 0;
    int forcedRatio = 0;
    int perfectRngScoreCutoff = 0;
    int goodRngScoreCutoff = 0;
    int perfectRngFicsSize = 0;
    int goodRngFicsSize = 0;
    int favouritesSize = 0;
    int largeListCounter = 0;

    QDate lastLargeListRegenerationDate;

    QString lastUsedRoll = QStringLiteral("all");

    std::chrono::system_clock::time_point lastRecsQuery;
    std::chrono::system_clock::time_point lastEasyQuery;
    std::chrono::system_clock::time_point lastActive;
    std::chrono::system_clock::time_point timeoutLimit;

    QSharedPointer<core::RecommendationListFicData> fics;
    QSet<int> perfectRngFics;
    QSet<int> goodRngFics;

    QSet<int> ignoredFics;
    FandomFilter filteredFandoms;
    WordcountFilter wordcountFilter;
    FandomFilter ignoredFandoms;
    QHash<int, int> positionToId;
    ECommandType lastPageType = ct_display_page;
    LastPageCommandMemo lastPageCommandMemo;
    LargeListToken largeListToken;
    mutable QReadWriteLock lock;
};


struct Users{
    void AddUser(QSharedPointer<User>);
    void RemoveUserData(QSharedPointer<User>);
    bool HasUser(QString);
    QSharedPointer<User> GetUser(QString) const;
    bool LoadUser(QString user);
    void ClearInactiveUsers();
    QHash<QString,QSharedPointer<User>> users;
    QReadWriteLock lock;

    //QSharedPointer<interfaces::Users> userInterface;
};
}

BIND_TO_SELF_SINGLE(discord::Users);
