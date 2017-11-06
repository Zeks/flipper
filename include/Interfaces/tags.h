#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {
class Tags{
public:
    void LoadAlltags();
    void EnsureTag(QString tag);
    bool DeleteTag(QString);
    bool CreateTag(QString);
    QStringList ReadUserTags();
    bool SetTagForFic(int ficId, QString tag);
    bool RemoveTagFromFic(int ficId, QString tag);
QSqlDatabase db;
QSharedPointer<Fandoms> fandomInterface;
private:
    QStringList CreateDefaultTagList();
    QStringList allTags;



};


}
