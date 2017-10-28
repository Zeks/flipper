#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include <QScopedPointer>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QReadWriteLock>
#include <QSet>


namespace interfaces {
class Genres{
public:
    
    bool IsGenreList(QStringList list);
    bool LoadGenres();

private:
    QSqlDatabase db;
    QSet<QString> genres;
};


}
