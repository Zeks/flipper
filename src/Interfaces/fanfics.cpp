#include "Interfaces/fanfics.h"
#include "Interfaces/authors.h"
#include "include/pure_sql.h"
#include <QVector>

namespace interfaces {


Fanfics::~Fanfics()
{

}

int Fanfics::GetIDFromWebID(int, QString website)
{
    return -1;
}

int Fanfics::GetWebIDFromID(int, QString website)
{
    return -1;
}

bool Fanfics::ReprocessFics(QString where, QString website, std::function<void (int)> f)
{
    auto list = database::puresql::GetWebIdList(where, website, db);
    if(list.empty())
        return false;
    for(auto id : list)
    {
        f(id);
    }
    return true;
}
bool Fanfics::IsEmptyQueues()
{
    return !(updateQueue.size() || insertQueue.size());
}
bool Fanfics::DeactivateFic(int ficId, QString website)
{
    return database::puresql::DeactivateStory(ficId, website, db);
}
void Fanfics::AddRecommendations(QList<core::FicRecommendation> recommendations)
{
    QWriteLocker lock(&mutex);
    ficRecommendations.reserve(ficRecommendations.size() + recommendations.size());
    ficRecommendations += recommendations;
}
void Fanfics::ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert)
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
void Fanfics::CalculateFandomAverages()
{
    database::puresql::CalculateFandomAverages(db);
}

void Fanfics::CalculateFandomFicCounts()
{
    database::puresql::CalculateFandomFicCounts(db);
}

void Fanfics::FlushDataQueues()
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

QList<core::Fic> Fanfics::GetCurrentFicSet()
{
    return currentSet;
}

}
