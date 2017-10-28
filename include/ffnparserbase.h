#pragma once
#include "include/section.h"
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"

#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>

namespace interfaces {
class Fanfics;
class Authors;
}

class FFNParserBase
{
public:
    FFNParserBase(){}
    FFNParserBase(QSharedPointer<interfaces::Fanfics> fanfics){
        this->fanfics = fanfics;
    }
    virtual ~FFNParserBase();
    void ProcessGenres(core::Section & section, QString genreText);
    void ProcessCharacters(core::Section & section, QString genreText);
    virtual void WriteProcessed() = 0;
    virtual void ClearProcessed() = 0;



    QSharedPointer<interfaces::Fanfics> fanfics;
    QList<QSharedPointer<core::Fic>> processedStuff;
    QStringList diagnostics;
};

