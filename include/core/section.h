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
#include "core/fav_list_details.h"
#include "core/recommendation_list.h"
namespace core {

class FavouritesPage
{
public:
    QSharedPointer<Author> author;
    QString pageData;
    //type of website, ffn or ao3

};
class Fic;




struct FandomStatsForWeightCalc{
  int listId = -1;
  int fandomCount = -1;
  int ficCount = -1;

  double fandomDiversity = 0.0;

  QHash<int, double> fandomPresence;
  QHash<int, int> fandomCounts;


  void Serialize(QDataStream &out)
  {
        out << listId;
        out << fandomCount;
        out << ficCount;
        out << fandomDiversity;

        out << fandomPresence;
        out << fandomCounts;
  }

  void Deserialize(QDataStream &in)
  {
      in >> listId;
      in >> fandomCount;
      in >> ficCount;
      in >> fandomDiversity;

      in >> fandomPresence;
      in >> fandomCounts;

  }

};


struct FicWeightResult{
    int ficId1;
    int ficId2;
    int ficListCount1;
    int ficListCount2;
    int meetingCount;
    bool sameFandom;
    double attraction;
    double repulsion;
    double finalAttraction;
};

typedef QSharedPointer<FicDataForRecommendationCreation> FicWeightPtr;
typedef QSharedPointer<FandomStatsForWeightCalc> AuthorFavFandomStatsPtr;

class Section : public DBEntity
{
public:
    Section();
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
    struct StatSection{
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

    StatSection statSection;
    QSharedPointer<Fic> result;
    bool isValid =false;
};
class Fandom;
typedef  QSharedPointer<Fandom> FandomPtr;

class Url
{
public:
    Url(QString url, QString source, QString type = "default"){
        this->url = url;
        this->source = source;
        this->type = type;
    }
    QString GetUrl(){return url;}
    QString GetSource(){return source;}
    QString GetType(){return type;}
    void SetType(QString value) {type = value;}
private:
    QString url;
    QString source;
    QString type;
};

class Fandom : public DBEntity
{
public:
    Fandom(){}
    Fandom(QString name){this->name = ConvertName(name);}
    //    Fandom(QString name,QString section,QString source = "ffn"){
    //        this->name = ConvertName(name);
    //        this->section = section.trimmed();
    //        this->source = source.trimmed();
    //    }
    static FandomPtr NewFandom() { return QSharedPointer<Fandom>(new Fandom);}
    QList<Url> GetUrls(){
        return urls;
    }
    void AddUrl(Url url){
        urls.append(url);
    }
    void SetName(QString name){this->name = ConvertName(name);}
    QString GetName() const;
    int id = -1;
    int idInRecentFandoms = -1;
    int ficCount = 0;
    double averageFavesTop3 = 0.0;

    QString section = "none";
    QString source = "ffn";
    QList<Url> urls;
    QDate dateOfCreation;
    QDate dateOfFirstFic;
    QDate dateOfLastFic;
    QDate lastUpdateDate;
    bool tracked = false;
    static QString ConvertName(QString name)
    {
        thread_local QHash<QString, QString> cache;
        name=name.trimmed();
        QString result;
        if(cache.contains(name))
            result = cache[name];
        else
        {
            QRegExp rx = QRegExp("(/(.|\\s){0,}[^\\x0000-\\x007F])|(/(.|\\s){0,}[?][?][?])");
            rx.setMinimal(true);
            int index = name.indexOf(rx);
            if(index != -1)
                cache[name] = name.left(index).trimmed();
            else
                cache[name] = name.trimmed();
            result = cache[name];
        }
        return result;
    }
private:
    QString name;

};

struct MatchedFics{
    int ratio = 0;
    int ratioWithoutIgnores = 0;
    QList<int> matches;
};

struct SnoozeInfo{
    int ficId = -1;
    bool finished = false;
    int atChapter = -1;
};
struct SnoozeTaskInfo{
    int ficId = -1;
    bool untilFinished = false;
    int snoozedAtChapter = -1;
    int snoozedTillChapter = -1;
    bool expired = false;
    QDateTime added;
};
}


