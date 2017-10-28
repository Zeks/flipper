#include "Interfaces/fanfics.h"
#include "Interfaces/authors.h"
#include "include/pure_sql.h"
#include <QVector>

namespace database {


DBFanficsBase::~DBFanficsBase()
{

}

int DBFanficsBase::GetIDFromWebID(int, QString website)
{
    return -1;
}

int DBFanficsBase::GetWebIDFromID(int, QString website)
{
    return -1;
}

bool DBFanficsBase::ReprocessFics(QString where, QString website, std::function<void (int)> f)
{
    auto list = puresql::GetWebIdList(where, website, db);
    if(list.empty())
        return false;
    for(auto id : list)
    {
        f(id);
    }
    return true;
}
bool DBFanficsBase::IsEmptyQueues()
{
    return !(updateQueue.size() || insertQueue.size());
}
bool DBFanficsBase::DeactivateFic(int ficId, QString website)
{
    return puresql::DeactivateStory(ficId, website, db);
}
void DBFanficsBase::AddRecommendations(QList<core::FicRecommendation> recommendations)
{
    QWriteLocker lock(&mutex);
    ficRecommendations.reserve(ficRecommendations.size() + recommendations.size());
    ficRecommendations += recommendations;
}
void DBFanficsBase::ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert)
{
    for(QSharedPointer<core::Fic> fic: fics)
    {
        if(!fic)
            continue;
        auto id = fic->id;
        database::puresql::SetUpdateOrInsert(fic, db, alwaysUpdateIfNotInsert);
        {
            QWriteLocker lock(&mutex);
            if(fic->updateMode == core::UpdateMode::update && !updateQueue.contains(fic->id))
                updateQueue[id] = fic;
            if(fic->updateMode == core::UpdateMode::insert && !insertQueue.contains(fic->id))
                insertQueue[id] = fic;
        }
    }

}
void DBFanficsBase::CalculateFandomAverages()
{
    database::puresql::CalculateFandomAverages(db);
}

void DBFanficsBase::CalculateFandomFicCounts()
{
    database::puresql::CalculateFandomFicCounts(db);
}

void DBFanficsBase::FlushDataQueues()
{
    db.transaction();
    for(auto fic: insertQueue)
        database::puresql::InsertIntoDB(fic, db);

    for(auto fic: insertQueue)
        database::puresql::UpdateInDB(fic, db);

    for(auto recommendation: ficRecommendations)
    {
        if(!recommendation.IsValid() || !authorInterface->EnsureId(recommendation.author))
            continue;
        auto id = GetIDFromWebID(recommendation.fic->webId, recommendation.fic->webSite);
        database::puresql::WriteRecommendation(recommendation.author, id, db);
    }
    db.commit();
}

QList<core::Fic> DBFanficsBase::GetCurrentFicSet()
{
    return currentSet;
}

}
