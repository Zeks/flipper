/*
FFSSE is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
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

