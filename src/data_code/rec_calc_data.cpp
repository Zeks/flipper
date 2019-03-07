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
