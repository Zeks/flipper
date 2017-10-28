#pragma once
#include "include/section.h"
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"

#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>



class FFNParserBase
{
public:
    virtual ~FFNParserBase();
    void ProcessGenres(core::Section & section, QString genreText);
    void ProcessCharacters(core::Section & section, QString genreText);
    virtual void WriteProcessed() = 0;
    virtual void ClearProcessed() = 0;



    QSharedPointer<interfaces::DataInterfaces> interfaces;
    QStringList diagnostics;
    QList<QSharedPointer<core::Fic>> processedStuff;
    QSqlDatabase db;
};

