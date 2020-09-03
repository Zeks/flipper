#include "discord/discord_user.h"
#include "sql/discord/discord_queries.h"
using namespace std::chrono;
namespace discord{
User::User(QString userID, QString ffnID, QString name)
{
    InitFicsPtr();
    this->userID = userID;
    this->ffnID = ffnID;
    this->userName = name;
}

User::User(const User &other)
{
    this->InitFicsPtr();
    *this = other;
}

void User::InitFicsPtr()
{
    fics = QSharedPointer<core::RecommendationListFicData>{new core::RecommendationListFicData};
}

User& User::operator=(const User &user)
{
    // self-assignment guard
    if (this == &user)
        return *this;
    this->userID = user.userID;
    this->ffnID = user.ffnID;
    this->page = user.page;
    this->banned = user.banned;
    this->lastRecsQuery = user.lastRecsQuery;
    this->lastEasyQuery = user.lastEasyQuery;
    // return the existing object so we can chain this operator
    return *this;
}

int User::secsSinceLastsRecQuery()
{
    QReadLocker locker(&lock);
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastRecsQuery).count());
}

int User::secsSinceLastsEasyQuery()
{
    QReadLocker locker(&lock);
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastEasyQuery).count());
}

int User::secsSinceLastsQuery()
{
    QReadLocker locker(&lock);
    if(lastRecsQuery > lastEasyQuery)
        return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastRecsQuery).count());
    else
        return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastEasyQuery).count());

}


void User::initNewRecsQuery()
{
    QWriteLocker locker(&lock);
    lastRecsQuery = std::chrono::system_clock::now();
}

void User::initNewEasyQuery()
{
    QWriteLocker locker(&lock);
    lastEasyQuery = std::chrono::system_clock::now();
}

int User::CurrentPage() const
{
    QReadLocker locker(&lock);
    return page;
}

void User::SetPage(int newPage)
{
    QWriteLocker locker(&lock);
    page = newPage;
}

void User::AdvancePage(int value)
{
    QWriteLocker locker(&lock);
    lastEasyQuery = std::chrono::system_clock::now();
    page += value;
    if(page < 0)
        page = 0;
}

bool User::HasUnfinishedRecRequest() const
{
    QReadLocker locker(&lock);
    return hasUnfinishedRecRequest;
}

void User::SetPerformingRecRequest(bool value)
{
    QWriteLocker locker(&lock);
    hasUnfinishedRecRequest = value;
}

void User::SetCurrentListId(int listId)
{
    QWriteLocker locker(&lock);
    this->listId = listId;
}

void User::SetBanned(bool banned)
{
    QWriteLocker locker(&lock);
    this->banned = banned;
}

void User::SetFfnID(QString id)
{
    QWriteLocker locker(&lock);
    this->ffnID = id;
}

void User::SetUserID(QString id)
{
    QWriteLocker locker(&lock);
    this->userID = id;
}

void User::SetUserName(QString name)
{
    QWriteLocker locker(&lock);
    this->userName = name;
}

void User::ToggleFandomIgnores(QSet<int> set)
{
    QWriteLocker locker(&lock);
    for(auto fandom: set)
    {
        if(!ignoredFandoms.contains(fandom))
            ignoredFandoms.insert(fandom);
        else
            ignoredFandoms.remove(fandom);
    }
}

QString User::FfnID()
{
    QReadLocker locker(&lock);
    return ffnID;
}

QString User::UserName()
{
    QReadLocker locker(&lock);
    return userName;
}

QString User::UserID()
{
    QReadLocker locker(&lock);
    return userID;
}

bool User::ReadsSlash()
{
    QReadLocker locker(&lock);
    return readsSlash;
}

bool User::HasActiveSet()
{
    QReadLocker locker(&lock);
    return fics->matchCounts.size() > 0;
}

void User::SetFicList(core::RecommendationListFicData fics)
{
    QWriteLocker locker(&lock);
    *this->fics = fics;
}

QSharedPointer<core::RecommendationListFicData> User::FicList()
{
    return fics;
}
void Users::AddUser(QSharedPointer<User> user)
{
    QWriteLocker locker(&lock);
    users[user->UserID()] = user;
}

bool Users::HasUser(QString user)
{
    QReadLocker locker(&lock);

    if(users.contains(user))
        return true;
    return false;
}

QSharedPointer<User> Users::GetUser(QString user)
{
    return users[user];
}

bool Users::LoadUser(QString name)
{
    auto user = database::discord_quries::GetUser(userInterface->db, name).data;
    if(!user)
        return false;

    users[name] = user;
    return true;
}

void Users::InitInterface(QSqlDatabase db)
{
    userInterface = QSharedPointer<interfaces::Users>{new interfaces::Users};
    userInterface->db = db;
}
}
