/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QRegExp>
#include <QHash>
#include <QTextStream>
#include <QDataStream>
#include <QSet>
#include <array>
#include "reclist_author_result.h"
#include "core/fic_genre_data.h"
#include "core/identity.h"
#include "core/db_entity.h"
#include "core/fanfic.h"
#include "core/author.h"
#include "core/url.h"
#include "core/fav_list_details.h"
#include "core/recommendation_list.h"
namespace core {

class FanficSectionInFFNFavourites : public DBEntity
{
public:
    FanficSectionInFFNFavourites();
    struct Tag
    {
        Tag(){}
        Tag(QString value, int end){
            this->value = value;
            this->end = end;
            isValid = true;
        }
        QString marker;
        QString value;
        int position = -1;
        int end = -1;
        bool isValid = false;
    };
    struct StatisticsLine{
        // these are tagged if they appear
        Tag rated;
        Tag reviews;
        Tag chapters;
        Tag words;
        Tag favs;
        Tag follows;
        Tag published;
        Tag updated;
        Tag status;
        Tag id;

        // these can only be inferred based on their content
        Tag genre;
        Tag language;
        Tag characters;

        QString text;
        bool success = false;
    };

    int start = 0;
    int end = 0;

    int summaryEnd = 0;
    int wordCountStart = 0;
    int statSectionStart=0;
    int statSectionEnd=0;

    StatisticsLine statSection;
    QSharedPointer<Fanfic> result;
    bool isValid =false;
};




}


