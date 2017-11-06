#include <algorithm>
#include <QSqlQuery>
#include <QVariant>
#include <QDateTime>


#include "Interfaces/fandoms.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include "include/section.h"
#include "GlobalHeaders/run_once.h"
#include "include/transaction.h"
#include "include/container_utils.h"


namespace interfaces {

void Fandoms::Clear()
{
    this->recentFandoms.clear();
    this->fandoms.clear();
    this->updateQueue.clear();
    this->nameIndex.clear();
    this->idIndex.clear();
    this->trackedFandoms.clear();
    this->fandomsList.clear();
}

void Fandoms::ClearIndex()
{
    this->nameIndex.clear();
    this->idIndex.clear();
    this->trackedFandoms.clear();
    this->fandomsList.clear();
}

bool Fandoms::EnsureFandom(QString name)
{
    if(nameIndex.contains(name) || LoadFandom(name))
        return true;
    return false;
}

bool Fandoms::CreateFandom(core::FandomPtr fandom)
{
    if(!fandom)
        return false;

    // getting empty node if its not there which is not a problem since we are
    // supposed to fill it at the end
    auto current = nameIndex[fandom->name];
    if(current && current->id != -1)
        return true;
    database::Transaction transaction(db);
    auto result = database::puresql::CreateFandomInDatabase(fandom, db);

    if(!result)
        return false;

    fandom->id = portableDBInterface->GetLastIdForTable("fandoms");
    if(!transaction.finalize())
        return false;
    AddToIndex(fandom);
    return true;
}

core::FandomPtr Fandoms::GetFandom(QString name)
{
    core::FandomPtr result;
    if(EnsureFandom(name))
        result = nameIndex[name];

    return result;
}

QStringList Fandoms::GetRecentFandoms()
{
    CleanupPtrList(recentFandoms);

    QStringList result;
    result.reserve(recentFandoms.size());

    std::sort(std::begin(recentFandoms), std::end(recentFandoms),[](auto f1, auto f2)->bool{
        return f1->idInRecentFandoms < f2->idInRecentFandoms;
    });

    std::for_each(std::begin(recentFandoms), std::end(recentFandoms), [&result](core::FandomPtr f){
        result.push_back(f->name);
    });
    return result;
}

void Fandoms::FillFandomList(bool forced)
{
    if(forced || fandomsList.isEmpty())
        fandomsList  = database::puresql::GetFandomListFromDB(db);
}


QStringList Fandoms::GetFandomList()
{
    FillFandomList();
    return fandomsList;
}


bool Fandoms::AddToTopOfRecent(QString fandom)
{
    if(!EnsureFandom(fandom))
        return false;

    bool foundIterating = false;
    for(auto bit: recentFandoms)
    {
        if(bit->name != fandom)
            bit->idInRecentFandoms = bit->idInRecentFandoms+1;
        else
        {
            bit->idInRecentFandoms = 0;
            foundIterating = true;
        }
    }
    if(!foundIterating)
    {
        recentFandoms.push_back(nameIndex[fandom]);
        nameIndex[fandom]->idInRecentFandoms = 0;
    }
    return true;
}



void Fandoms::PushFandomToTopOfRecent(QString fandom)
{
    if(!EnsureFandom(fandom))
        return;

    AddToTopOfRecent(fandom);

    // needs to be done on Sync ideally
    // it's not a critical error to fail here, really
    database::Transaction transaction(db);
    portableDBInterface->PushFandomToTopOfRecent(fandom);
    portableDBInterface->RebaseFandomsToZero();
    transaction.finalize();
}

bool Fandoms::IsDataLoaded()
{
    return isLoaded;
}

QList<core::FandomPtr> Fandoms::FilterFandoms(std::function<bool (core::FandomPtr)> f)
{
    QList<core::FandomPtr > result;
    if(!LoadAllFandoms())
        return result;

    result.reserve(fandoms.size()/2);
    for(auto fandom: fandoms)
    {
        if(f(fandom))
            result.push_back(fandom);
    }
    return result;
}

// do I really need that?
bool Fandoms::Sync(bool forcedSync)
{
    bool ok = true;
    for(auto fandom: fandoms)
    {
        if(forcedSync || fandom->hasChanges)
        {
            ok = ok && database::puresql::WriteMaxUpdateDateForFandom(fandom, db);
        }
    }
    return ok;
}

bool Fandoms::Load()
{
    Clear();
    // type makes it clearer
    QStringList recentFandoms = portableDBInterface->FetchRecentFandoms();
    bool hadErrors = false;

    // not loading every fandom in this function
    // there is no point in doing that until its actually necessary for
    // some service operation
    // need to access the recent ones for sure though
    for(auto bit: recentFandoms)
    {
        bool fandomPresent = false;
        if(EnsureFandom(bit))
            fandomPresent = true;
        else
            hadErrors = true;

        if(fandomPresent)
            this->recentFandoms.push_back(nameIndex[bit]);
    }
    fandomCount = database::puresql::GetFandomCountInDatabase(db);
    return hadErrors;
}

bool Fandoms::LoadTrackedFandoms(bool forced)
{
    bool result = false;
    bool needsLoading = trackedFandoms.empty()  || forced;
    if(!needsLoading)
        return true;

    auto trackedList = database::puresql::GetTrackedFandomList(db);
    if(trackedList.empty())
        return false;

    trackedFandoms.clear();
    trackedFandoms.reserve(trackedList.size());
    for(auto bit : trackedList)
    {
        bool localResult = EnsureFandom(bit);
        if(localResult)
            trackedFandoms.push_back(nameIndex[bit]);
        result = result && localResult;
    }
    return result;
}

bool Fandoms::LoadAllFandoms(bool forced)
{
    bool needsLoading = forced || fandoms.isEmpty();
    if(!needsLoading)
        return true;

    fandoms = database::puresql::GetAllFandoms(db);
    if(fandoms.empty())
        return false;
    return true;
}

bool Fandoms::IsTracked(QString fandom)
{
    if(!EnsureFandom(fandom))
        return false;
    return nameIndex[fandom]->tracked;
}

void Fandoms::Reindex()
{
    ClearIndex();
    nameIndex.reserve(fandoms.size());
    trackedFandoms.reserve(fandoms.size()/10);
    fandomsList.reserve(fandoms.size());
    idIndex.reserve(fandoms.size());
    for(auto fandom : fandoms)
        AddToIndex(fandom);
}

void Fandoms::AddToIndex(core::FandomPtr fandom)
{
    if(!fandom)
        return;
    nameIndex[fandom->name] = fandom;
    idIndex[fandom->id] = fandom;
    fandomsList.push_back(fandom->name);
    if(fandom->tracked)
        trackedFandoms.push_back(fandom);
}

bool Fandoms::WipeFandom(QString name)
{
    if(!EnsureFandom(name))
        return false;
    return database::puresql::CleanuFandom(nameIndex[name]->id, db);
}

int Fandoms::GetFandomCount()
{
    return fandomCount;
}

Fandoms::~Fandoms()
{

}

int Fandoms::GetIDForName(QString fandom)
{
    int result = -1;
    if(EnsureFandom(fandom))
        result = nameIndex[fandom]->id;
    return result;
}

void Fandoms::SetTracked(QString fandom, bool value, bool immediate)
{
    if(!EnsureFandom(fandom))
        return;

    if(immediate)
    {
        auto id = nameIndex[fandom]->id;
        database::puresql::SetFandomTracked(id, value, db);
    }
    else
        nameIndex[fandom]->hasChanges = nameIndex[fandom]->tracked == false;

    nameIndex[fandom]->tracked = value;
}

QStringList Fandoms::ListOfTrackedNames()
{
    QStringList names;
    if(!LoadTrackedFandoms())
        return names;
    for(auto fandom: trackedFandoms)
        if(fandom)
            names.push_back(fandom->name);
    return names;
}

QList<core::FandomPtr > Fandoms::ListOfTrackedFandoms()
{
    LoadTrackedFandoms();
    return trackedFandoms;
}



bool Fandoms::LoadFandom(QString name)
{
    auto fandom = database::puresql::GetFandom(name, db);
    if(!fandom)
        return false;

    fandoms.push_back(fandom);
    AddToIndex(fandom);
    return true;
}

bool Fandoms::AssignTagToFandom(QString fandom, QString tag)
{
    if(!EnsureFandom(fandom))
        return false;

    auto id = nameIndex[fandom]->id;
    database::puresql::AssignTagToFandom(tag, id, db);
    return true;
}

void Fandoms::CalculateFandomAverages()
{
    database::puresql::CalculateFandomAverages(db);
}

void Fandoms::CalculateFandomFicCounts()
{
    database::puresql::CalculateFandomFicCounts(db);
}

}
