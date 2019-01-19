#pragma once
#include "include/data_code/data_holders.h"
namespace core{
    
    
    
struct DataHolder
{
    typedef DataHolderInfo<rdt_favourites>::type FavType;
    typedef DataHolderInfo<rdt_fics>::type FicType;
    typedef DataHolderInfo<rdt_author_genre_distribution>::type GenreType;
    DataHolder(QString settingsFile,
               QSharedPointer<interfaces::Authors> authorsInterface,
               QSharedPointer<interfaces::Fanfics> fanficsInterface)
        :settingsFile(settingsFile),
          authorsInterface(authorsInterface),
          fanficsInterface(fanficsInterface){}

    template <std::size_t I>
    auto get()
    {
        if constexpr (I == rdt_favourites)
                return std::tuple{std::ref(this->faves), this->authorsInterface};
        else if constexpr (I == rdt_fics)
                return std::tuple{std::ref(this->fics), this->fanficsInterface};
        else if constexpr (I == rdt_author_genre_distribution)
                return std::tuple{std::ref(this->genres), this->authorsInterface};
    }

    template <ERecDataType T>
    void LoadData(QString storageFolder);

    template <ERecDataType T>
    void SaveData(QString storageFolder){
        CreateTempDataDir(storageFolder);
        auto[data, interface] = get<T>();
        QString fileBase = QString::fromStdString(DataHolderInfo<T>::fileBase());
        thread_boost::SaveData(storageFolder, fileBase, data);
    }
    void CreateTempDataDir(QString storageFolder)
    {
        QDir dir(QDir::currentPath());
        dir.mkdir(storageFolder);
    }

    QString settingsFile;
    QSharedPointer<interfaces::Authors> authorsInterface;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;

    FavType faves;
    GenreType genres;
    FicType fics;
};
    
}