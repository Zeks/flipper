#include "Interfaces/fandoms.h"

namespace database {

void DBFandomsBase::Clear()
{
    this->recentFandoms.clear();
    this->fandoms.clear();
    this->indexFandomsByName.clear();
    this->updateQueue->clear();
}

bool DBFandomsBase::EnsureFandom(QString name)
{
    if(!fandoms.contains(fandom) && !LoadFandom(fandom))
        return false;
    return true;
}

bool DBFandomsBase::CreateFandom(QSharedPointer<core::Fandom> fandom)
{
    if(fandoms.contains(fandom->name) && fandoms[fandom->name]->id != -1)
        return true;

    auto result = database::puresql::CreateFandomInDatabase(fandom, db);

    if(!result)
        return false;
    fandom->id = portableDBInterface->GetLastIdForTable("fandoms", db);
    EnsureFandom(fandom->name);
    return true;
}

QSharedPointer<core::Fandom> DBFandomsBase::GetFandom(QString name)
{
    QSharedPointer<core::Fandom> result;
    if(!EnsureFandom(name))
        return result;

    return fandoms[name];
}

QStringList DBFandomsBase::GetRecentFandoms() const
{
    QStringList result;
    std::sort(std::begin(recentFandoms), std::end(recentFandoms), [](QSharedPointer<core::Fandom> f1, QSharedPointer<core::Fandom> f2){
        return f1->idInRecentFandoms < f1->idInRecentFandoms;
    });
    std::for_each(std::begin(recentFandoms), std::end(recentFandoms), [&result](QSharedPointer<core::Fandom> f){
        result.push_back(f->name);
    });
    return result;
}


void DBFandomsBase::AddToTopOfRecent(QString fandom)
{
    if(!fandoms.contains(fandom))
        return;
    bool updatedThis = false;
    for(auto bit: recentFandoms)
    {
        if(bit->name != fandom)
            bit->idInRecentFandoms = bit->idInRecentFandoms+1;
        else
            bit->idInRecentFandoms = 0;
    }
    if(!updatedThis)
    {
        recentFandoms.push_back(fandoms[fandom]);
        fandoms[fandom]->idInRecentFandoms = 0;
    }
}


QStringList DBFandomsBase::PushFandomToTopOfRecent(QString fandom)
{
    QStringList result = GetRecentFandoms();
    if(!fandoms.contains(fandom))
        return result;
    // needs to be done on Sync ideally
    portableDBInterface->PushFandomToTopOfRecent(fandom, db);
    portableDBInterface->RebaseFandomsToZero(db);
    AddToTopOfRecent(fandom);
    result = GetRecentFandoms();
    return result;

}


bool DBFandomsBase::IsDataLoaded()
{
    return isLoaded;
}

bool DBFandomsBase::Sync(bool forcedSync)
{
    for(auto fandom: fandoms)
    {
        if(fandom->hasChanges || forcedSync)
        {
            database::puresql::WriteMaxUpdateDateForFandom(fandom, db);
        }
    }
}

bool DBFandomsBase::Load()
{
    Clear();
    auto recentFandoms = portableDBInterface->FetchRecentFandoms(db);
    for(auto bit: recentFandoms)
    {
        bool fandomPresent = false;
        if(fandoms.contains(bit) || LoadFandom(bit))
           fandomPresent = true;

        if(fandomPresent)
            this->recentFandoms.push_back(fandoms[bit]);
    }
}

bool DBFandomsBase::IsTracked(QString fandom)
{
    bool tracked = false;
    if(IsDataLoaded())
    {
        if(fandoms.contains(fandom))
            tracked = fandoms[fandom].tracked;
    }
    else
    {
        QSqlQuery q1(db);
        QString qsl = " select tracked from fandoms where id = :id ";
        auto id = GetID(fandom);
        q1.prepare(qsl);
        q1.bindValue(":id",id);
        ExecAndCheck(q1);
        q1.next();
        tracked = q1.value(0).toBool();
    }
    return tracked;
}

QList<int> DBFandomsBase::AllTracked()
{
    QList<int> result;
    if(IsDataLoaded())
    {
        result.reserve(fandoms.size());
        for(auto &fandom: fandoms)
            if(fandom.tracked)
                result.push_back(fandom.id);
    }
    else
    {
        QSqlQuery q1(db);
        QString qsl = " select fandom from fandoms where tracked = 1";
        q1.prepare(qsl);
        q1.exec();
        QStringList result;
        while(q1.next())
            result.push_back(q1.value(0).toString());

        CheckExecution(q1);
        return result;
    }
}

QStringList DBFandomsBase::AllTrackedStr()
{
    QStringList result;
    auto tracked = AllTracked();
    for(auto bit : tracked)
        result.push_back(QString::number(bit));
    return result;
}

int DBFandomsBase::GetID(QString value)
{
    if(!indexFandomsByName.contains(value))
        return -1;
    return indexFandomsByName[value];
}

void DBFandomsBase::SetTracked(QString fandom, bool value, bool immediate)
{
    if(!fandoms.contains(fandom) && !LoadFandom(fandom))
        return;

    if(immediate)
    {
        auto id = fandoms[fandom]->id;
        database::puresql::SetFandomTracked(id, value, db);
    }
    else
        fandoms[fandom]->hasChanges = fandoms[fandom]->tracked == false;

    fandoms[fandom]->tracked = value;
}

bool DBFandomsBase::IsTracked(QString name)
{
    if(!fandoms.contains(name) || !fandoms[name])
        return false;
    return fandoms[name]->tracked;
}

QStringList DBFandomsBase::ListOfTracked()
{
    QStringList names;
    for(auto fandom: fandoms)
        if(fandom && fandom->tracked)
            names.push_back(fandom->name);
    return names;
}



bool DBFandomsBase::LoadFandom(QString name)
{
    QSharedPointer<core::Fandom> fandom(new core::Fandom);
    QSqlQuery q(db);
    q.prepare("select * from fandoms where fandom = :fandom_name and source = 'ffn'");
    if(!ExecAndCheck(q))
        return false;
    q.next();

    //crossoverurl is subject to weird changes so its better to load that on first fandom read each time
    fandom->name = name;
    fandom->url = q.value("NORMAL_URL").toString();
    fandom->crossoverUrl = q.value("CROSSOVER_URL").toString() + "/0/"; //!todo fix it
    fandom->dateOfCreation = q.value("date_of_first_fic").toString();
    fandom->dateOfLastFic= q.value("date_of_last_fic").toString();
    fandom->lastUpdateDate = q.value("last_update").toString();
    fandom->lastCrossoverUpdateDate = q.value("last_update_crossover").toString();
    fandom->id = q.value("id").toInt();
    fandom->section = q.value("section").toString();
    fandom->tracked = q.value("tracked").toBool();
    fandom->ficCount = q.value("fic_count").toInt();
    fandom->averageFavesTop3 = q.value("average_faves_top_3").toInt();
    fandoms[name] = fandom;
    return true;
}

bool DBFandomsBase::AssignTagToFandom(QString fandom, QString tag)
{
    if(!EnsureFandom(name))
        return false;
    auto id = fandoms[fandom]->id;
    database::puresql::AssignTagToFandom(tag, id, db);
}

bool DBFandomsBase::CreateFandom(QSharedPointer<core::Fandom> fandom)
{
    if(fandoms.contains(fandom->name) && fandoms[fandom->name]->id != -1)
        return true;

    auto result = database::puresql::CreateFandomInDatabase(fandom, db);

    if(!result)
        return false;
    fandom->id = portableDBInterface->GetLastIdForTable("fandoms", db);
    EnsureFandom(fandom->name);
    return true;
}


QString DBFandomsBase::GetCurrentCrossoverUrl()
{
    //will need to parse the page for that
}

}
