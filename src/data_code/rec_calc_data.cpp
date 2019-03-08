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
#include "include/data_code/rec_calc_data.h"

#include <QSettings>
#include <QFileInfo>



namespace core{

auto lambda = [](DataHolder* holder, QString storageFolder, QString fileBase, auto& data, auto interface, auto loadFunc, auto saveFunc)->void{
    holder->CreateTempDataDir(storageFolder);
    QSettings settings(holder->settingsFile, QSettings::IniFormat);
    QFileInfo fi;
    if(settings.value("Settings/usestoreddata", true).toBool() && fi.exists(storageFolder + "/" + fileBase + "_0.txt"))
        thread_boost::LoadData(storageFolder, fileBase, data);
    else
    {
        auto& item = data;
        item = loadFunc(interface);
        saveFunc(storageFolder);
    }
};

#define DISPATCH(X) \
    template <> \
    void DataHolder::LoadData<X>(QString storageFolder){ \
    auto[data, interface] = get<X>(); \
    lambda(this,storageFolder, QString::fromStdString(DataHolderInfo<X>::fileBase()), data.get(),interface, DataHolderInfo<X>::loadFunc(),\
    std::bind(&DataHolder::SaveData<X>, this, std::placeholders::_1));\
}

DISPATCH(rdt_favourites)
DISPATCH(rdt_fics)
DISPATCH(rdt_author_genre_distribution)
DISPATCH(rdt_author_mood_distribution)
DISPATCH(rdt_fic_genres_composite)

}
