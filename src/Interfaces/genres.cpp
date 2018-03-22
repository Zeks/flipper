/*
Flipper is a replacement search engine for fanfiction.net search results
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
#include "Interfaces/genres.h"
#include "pure_sql.h"

namespace interfaces {


QHash<QString, QString> CreateCodeToDBGenreConverter()
{
    QHash<QString, QString> result;
    result["General"] = "General_";
    result["Humor"] = "Humor";
    result["Poetry"] = "Poetry";
    result["Adventure"] = "Adventure";
    result["Mystery"] = "Mystery";
    result["Horror"] = "Horror";
    result["Parody"] = "Parody";
    result["Angst"] = "Angst";
    result["Supernatural"] = "Supernatural";
    result["Suspense"] = "Suspense";
    result["Romance"] = "Romance";
    result["not found"] = "NoGenre";
    result["Sci-Fi"] = "SciFi";
    result["Fantasy"] = "Fantasy";
    result["Spiritual"] = "Spiritual";
    result["Tragedy"] = "Tragedy";
    result["Drama"] = "Drama";
    result["Western"] = "Western";
    result["Crime"] = "Crime";
    result["Family"] = "Family";
    result["Hurt/Comfort"] = "HurtComfort";
    result["Friendship"] = "Friendship";
    return result;
}

QHash<QString, QString> CreateDBToCodeGenreConverter()
{
    QHash<QString, QString> result;
    result["HurtComfort"] = "Hurt/Comfort";
    result["General_"] = "General";
    result["NoGenre"] = "not found";
    result["SciFi"] = "Sci-Fi";
    result["Humor"] = "Humor";
    result["Poetry"] = "Poetry";
    result["Adventure"] = "Adventure";
    result["Mystery"] = "Mystery";
    result["Horror"] = "Horror";
    result["Parody"] = "Parody";
    result["Angst"] = "Angst";
    result["Supernatural"] = "Supernatural";
    result["Suspense"] = "Suspense";
    result["Romance"] = "Romance";
    result["Fantasy"] = "Fantasy";
    result["Spiritual"] = "Spiritual";
    result["Tragedy"] = "Tragedy";
    result["Drama"] = "Drama";
    result["Western"] = "Western";
    result["Crime"] = "Crime";
    result["Family"] = "Family";
    result["Friendship"] = "Friendship";
    return result;
}


bool interfaces::Genres::IsGenreList(QStringList list)
{
    bool success = false;
    for(auto token : list)
        if(genres.contains(token))
        {
            success = true;
            break;
        }
    return success;
}

bool Genres::LoadGenres()
{
    genres = database::puresql::GetAllGenres(db).data;
    if(genres.empty())
        return false;
    return true;
}

GenreConverter GenreConverter::Instance()
{
    thread_local GenreConverter instance;
    return instance;
}


}
