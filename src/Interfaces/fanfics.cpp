#include "Interfaces/fanfics.h"
#include "include/pure_sql.h"

namespace database {


DBFanficsBase::~DBFanficsBase()
{

}

bool DBFanficsBase::ReprocessFics(QString where, QString website, std::function<void (int)> f)
{
    auto list = puresql::GetWebIdList(where, website, db);
    for(auto id : list)
    {
        f(id);
    }
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
        database::InsertIntoDB(fic, db);

    for(auto fic: insertQueue)
        database::UpdateInDB(fic, db);

    for(auto recommendation: ficRecommendations)
    {
        if(!recommendation.IsValid() || !authorInterface->EnsureId(recommendation.author))
            continue;
        auto id = GetIDFromWebID(recommendation.fic->webId);
        database::WriteRecommendation(recommendation.author, id, db);
    }
    db.commit();
}

}
