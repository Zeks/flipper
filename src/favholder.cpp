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
#include "include/favholder.h"
#include "include/Interfaces/authors.h"
#include "logger/QsLog.h"
#include "include/timeutils.h"
#include "threaded_data/threaded_save.h"
#include "threaded_data/threaded_load.h"
#include <QSettings>
#include <QDir>
namespace core{

void FavHolder::LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface)
{
    CreateTempDataDir();
    QSettings settings("settings_server.ini", QSettings::IniFormat);
    if(settings.value("Settings/usestoreddata", false).toBool() && QFile::exists("ServerData/fav_0.txt"))
    {
        LoadStoredData();
    }
    else
    {
        LoadDataFromDatabase(authorInterface);
        SaveData();

    }
}

void FavHolder::CreateTempDataDir()
{
    QDir dir(QDir::currentPath());
    dir.mkdir("ServerData");
}

void FavHolder::LoadDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface)
{
    favourites = authorInterface->LoadFullFavouritesHashset();
}

void FavHolder::LoadStoredData()
{
    thread_boost::LoadFavouritesData("ServerData", favourites);
}

void FavHolder::SaveData()
{
    thread_boost::SaveFavouritesData("ServerData", favourites);
}

struct AuthorResult{
    int id;
    int matches;
    double ratio;
    int size;
};



RecommendationListResult FavHolder::GetMatchedFicsForFavList(QSet<int> sourceFics, QSharedPointer<RecommendationList> params)
{
    QHash<int, AuthorResult> authorsResult;
    RecommendationListResult ficResult;
    //QHash<int, int> ficResult;
    auto& favs = favourites;
    TimedAction action("Reclist Creation",[&](){
        auto it = favs.begin();
        while (it != favs.end())
        {
            auto& author = authorsResult[it.key()];
            author.id = it.key();
            author.matches = 0;
            author.ratio = 0;
            author.size = it.value().size();
            for(auto source: sourceFics)
            {
                if(it.value().contains(source))
                    author.matches = author.matches + 1;
            }
            //if(author.matches > 0)
                //QLOG_INFO() << " Author: " << it.key() << " had: " << author.matches << " matches";
            ++it;
        }
        for(auto& author: authorsResult)
        {
            if(author.matches > 0)
                author.ratio = static_cast<double>(author.size)/static_cast<double>(author.matches);
            if((author.matches >= params->minimumMatch && author.ratio <= params->pickRatio)
                    || author.matches >= params->alwaysPickAt)
            {
                for(auto fic: favourites[author.id])
                    ficResult.recommendations[fic]++;
            }
        }
    });
    action.run();
    for(auto author: authorsResult)
    {
        if((author.matches >= params->minimumMatch && author.ratio <= params->pickRatio)
                || author.matches >= params->alwaysPickAt)
        {
            ficResult.matchReport[author.matches]++;
        }
    }
    QList<AuthorResult> authors = authorsResult.values();
    std::sort(authors.begin(), authors.end(), [](const AuthorResult& a1, const AuthorResult& a2){
        return a1.matches > a2.matches;
    });
    qDebug() << "/////////////////// AUTHOR IDs: ////////////////";
    int counter = 0;
    for(int i = 0; counter < 15 && i < authors.size(); i++)
    {
        if(authors[i].size < 800)
        {
            qDebug() << authors[i].id;
            counter++;
        }
    }
    return ficResult;
}

}
