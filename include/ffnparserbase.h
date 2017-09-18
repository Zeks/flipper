#pragma once
#include "include/section.h"
#include "include/db_ffn.h"
#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>



class FFNParserBase
{
public:
    virtual ~FFNParserBase(){}
    void ProcessGenres(core::Section & section, QString genreText);
    void ProcessCharacters(core::Section & section, QString genreText);
   
    QStringList diagnostics;
    core::Fic processedStuff;
    database::WriteStats writeSections;
};

