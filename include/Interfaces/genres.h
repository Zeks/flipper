/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include "Interfaces/base.h"
#include "core/section.h"
#include "core/fic_genre_data.h"
#include "GlobalHeaders/SingletonHolder.h"
#include <QScopedPointer>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QReadWriteLock>
#include <QSet>
#include <optional>

namespace interfaces {

QHash<QString, QString> CreateCodeToDBGenreConverter();
QHash<QString, QString> CreateDBToCodeGenreConverter();

struct GenreConverter{
    QHash<QString, QString> codeToDB = CreateCodeToDBGenreConverter();
    QHash<QString, QString> dBToCode = CreateDBToCodeGenreConverter();
    QStringList GetDBGenres() { return dBToCode.keys();}
    QStringList GetCodeGenres() { return codeToDB.keys();}
    QString ToDB(QString value){
        QString result;
        if(codeToDB.contains(value))
            result = codeToDB[value];
        return result;
    }
    QString ToCode(QString value){
        QString result;
        if(dBToCode.contains(value))
            result = dBToCode[value];
        return result;
    }
    QStringList GetFFNGenreList(QString genreString){

        QStringList genresList;
        bool hasHurt = false;
        QString fixedGenreString = genreString;
        if(genreString.contains("Hurt/Comfort"))
        {
            hasHurt = true;
            fixedGenreString = fixedGenreString.replace("Hurt/Comfort", "");
        }

        genresList = fixedGenreString.split("/", QString::SkipEmptyParts);
        if(hasHurt)
            genresList.push_back("Hurt/Comfort");
        for(auto& genre: genresList)
            genre = genre.trimmed();
        return genresList;
    }

    void ProcessGenreResult(genre_stats::FicGenreData&);
    void DetectRealGenres(genre_stats::FicGenreData&);

    static GenreConverter Instance();
};

enum EGenreCategory{
    gc_none         = 0,
    gc_funny        = 1,
    gc_flirty       = 2,
    gc_neutral      = 3,
    gc_dramatic     = 4,
    gc_shocky       = 5,
    gc_hurty        = 6,
    gc_bondy        = 7,
};
enum EMoodType{
    mt_none = 0,
    mt_sad = 1,
    mt_neutral = 2,
    mt_happy = 3
};

struct Genre{
    bool isValid;
    size_t indexInDatabase;
    QString name;
    QString nameInDatabase;
    EMoodType moodType;
    EGenreCategory genreCategory;
};

struct GenreIndex
{
    GenreIndex();
    void Init();
    void InitGenre(const Genre&  genre);
    Genre& GenreByName(QString name);

    QHash<QString, Genre> genresByName;
    QHash<QString, Genre> genresByDbName;
    QHash<size_t, Genre> genresByIndex;
    QHash<int, QList<Genre>> genresByCategory;
    QHash<int, QList<Genre>> genresByMood;
    Genre nullGenre;
};


class Genres{
public:
    Genres();
    bool IsGenreList(QStringList list);
    bool LoadGenres();
    genre_stats::FicGenreData GetGenreDataForFic(int id);
    QVector<genre_stats::FicGenreData> GetGenreDataForQueuedFics();
    void QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists, int minFaves);
    bool WriteDetectedGenres(QVector<genre_stats::FicGenreData> fics);
    std::optional<size_t> GetGenreIndex(QString genre);

    GenreIndex index;
    QSqlDatabase db;
private:

    QSet<QString> genres;
};




}
BIND_TO_SELF_SINGLE(interfaces::GenreIndex);
