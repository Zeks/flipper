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
#include "threaded_data/threaded_load.h"
#include "threaded_data/common_traits.h"

#include <QThread>
#include <QDebug>
#include <QStringList>
#include <QDataStream>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
namespace thread_boost{
namespace  Impl{
template<class T, class Enable = void>
struct ContainerValueTypeGetter {
    typedef  typename T::value_type type;
};

template<class T>
struct ContainerValueTypeGetter<T, typename std::enable_if<is_hash<T>::value>::type> {
    typedef  typename T::iterator::value_type type;
};

template <typename ContainerType, typename ValueType>
inline void PutIntoContainer(ContainerType& container, ValueType& value, int& key){
    if constexpr(is_hash<ContainerType>::value)
            container[key] = value;
    else
    container.push_back(value);
}

auto fetchFunc =  [](auto& container, QDataStream& in)->void{
    //using ContainerType = std::remove_const_t<std::remove_volatile_t<std::remove_reference_t<decltype(container)>>>;
    using ContainerType = std::remove_const_t<std::remove_volatile_t<std::remove_reference_t<decltype(container)>>>;
    using ContainerValueType =  typename ContainerValueTypeGetter<ContainerType>::type;

    int key;
    if constexpr(is_hash<ContainerType>::value)
    in >> key;

    if constexpr(is_roaring<ContainerValueType>::value)
    {
        //using PtrType = ContainerValueType;

        QByteArray ba;
        in >> ba;
        //qDebug() << "read byte array is: " << ba;
        Roaring r = Roaring::readSafe(ba.constData(), ba.size());

        //qDebug() << "read roaring of size: " << r.cardinality();
        PutIntoContainer(container, r, key);
    }

    else if constexpr(is_serializable_pointer<ContainerValueType>())
    {
        using PtrType = ContainerValueType;
        using ValueType = typename PtrType::value_type;

        PtrType value (new ValueType());
        value->Deserialize(in);

        PutIntoContainer(container, value, key);
    }
    else
    {
        using ValueType = ContainerValueType;
        ValueType value;
        in >> value;

        PutIntoContainer(container, value, key);
    }
};


auto loaderFunc = [](
        QString nameBase,
auto resultCreator,
int file,
auto valueFetcher){
    auto resultHolder = resultCreator();
    QString fileName = QString("%1_%2.txt").arg(nameBase).arg(QString::number(file));
    QFile data(fileName);
    if (data.open(QFile::ReadOnly)) {
        QDataStream in(&data);
        int size;
        in >> size;
        qDebug() << "Starting file: " << fileName << " of size: " << size;
        resultHolder.reserve(size);
        for(int i = 0; i < size; i++)
        {
            if(i%10000 == 0)
                qDebug() << "processing entity: " << fileName << " " << i;
            valueFetcher(resultHolder, in);
        }
    }
    else
        qDebug() << "Could not open file: " << fileName;
    qDebug() << "finished file: " << fileName;
    return resultHolder;
};


auto loadMultiThreaded = [](auto loaderFunc, auto resultUnifier, QString nameBase,auto& destination){
    static QVector<QFuture<typename std::remove_reference<decltype(destination)>::type>> futures;
    for(int i = 0; i < QThread::idealThreadCount()-1; i++)
    {
        futures.push_back(QtConcurrent::run([nameBase,i, loaderFunc](){
            qDebug() << "loading file: " << i;
            return loaderFunc(nameBase, [](){return typename std::remove_reference<decltype(destination)>::type();},i);
        }));
    }
    for(auto future: futures)
    {
        future.waitForFinished();
    }
    qDebug() << "starting unification";
    for(auto future: futures)
    {
        resultUnifier(destination, future.result());
    }
    qDebug() << "finished unification, size:" << destination.size() ;

};

auto vectorUnifier = [](auto& dest, auto source){
    dest+=source;
};
auto hashUnifier = [](auto& dest, auto source){
    dest.unite(source);
};
auto genresFetchFunc = [](auto& container, QDataStream& in){
    int key;
    in >> key;
    typename std::remove_reference<decltype(container)>::type::iterator::value_type values;
    for(int i = 0; i < 22; i++)
        in >> values[i];
    container[key] = values;
};

auto ficFetchFunc = [](auto& container, QDataStream& in){
    using PtrType = typename std::remove_reference<decltype(container)>::type::value_type;
    using ValueType = typename std::remove_reference<decltype(container)>::type::value_type::value_type;

    PtrType tmp{new ValueType()};
    tmp->Deserialize(in);
    container.push_back(tmp);
};

auto fandomListsFetchFunc =  [](auto& container, QDataStream& in){
    int key;
    in >> key;
    using PtrType = typename std::remove_reference<decltype(container)>::type::iterator::value_type;
    using ValueType = typename std::remove_reference<decltype(container)>::type::iterator::value_type::value_type;
    PtrType value (new ValueType());
    value->Deserialize(in);
    container[key] = value;
};
auto favouritesFetchFunc = [](auto& container, QDataStream& in){
    int key;
    in >> key;
    typename std::remove_reference<decltype(container)>::type::iterator::value_type values;
    in >> values;
    container[key] = values;
};




auto ficLoader = std::bind(loaderFunc, std::placeholders::_1,
                           std::placeholders::_2,
                           std::placeholders::_3,
                           ficFetchFunc);
auto authorLoader = std::bind(loaderFunc, std::placeholders::_1,
                              std::placeholders::_2,
                              std::placeholders::_3,
                              ficFetchFunc);
auto favouritesLoader = std::bind(loaderFunc, std::placeholders::_1,
                                  std::placeholders::_2,
                                  std::placeholders::_3,
                                  favouritesFetchFunc);

auto fandomListsLoader = std::bind(loaderFunc, std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3,
                                   fandomListsFetchFunc);

auto genericLoader = std::bind(loaderFunc, std::placeholders::_1,
                               std::placeholders::_2,
                               std::placeholders::_3,
                               fetchFunc);

// special case, non standard fecther
auto genreLoader = std::bind(loaderFunc, std::placeholders::_1,
                             std::placeholders::_2,
                             std::placeholders::_3,
                             genresFetchFunc);
}

using namespace Impl;
void LoadFicWeightCalcData(QString storage, QVector<core::FicWeightPtr>& fics)
{
    qDebug() << "Loading favourites";
    loadMultiThreaded(genericLoader, vectorUnifier, storage + "/fics", fics);
}

void LoadAuthorsData(QString storage,QList<core::AuthorPtr> &authors)
{
    qDebug() << "Loading authors";
    loadMultiThreaded(genericLoader,vectorUnifier, storage + "/authors",authors);
}
void LoadFavouritesData(QString storage, QHash<int, QSet<int>>& favourites)
{
    qDebug() << "Loading favourites";
    loadMultiThreaded(genericLoader, hashUnifier, storage + "/fav", favourites);
}
void LoadFavouritesData(QString storage, QHash<int, Roaring> &favourites)
{
    qDebug() << "Loading favourites";
    loadMultiThreaded(genericLoader, hashUnifier, storage + "/roafav", favourites);
}

void LoadGenreDataForFavLists(QString storage, QHash<int, std::array<double, 22> >& genreData)
{
    qDebug() << "Loading authors";
    loadMultiThreaded(genreLoader ,hashUnifier, storage + "/genre",genreData);
}
void LoadFandomDataForFavLists(QString storage, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists)
{
    qDebug() << "Loading favourites";
    loadMultiThreaded(genericLoader, hashUnifier, storage + "/fandomstats", fandomLists);
}


void LoadData(QString storageFolder, QString fileName, QHash<int, Roaring>& data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
void LoadData(QString storageFolder, QString fileName, QHash<int, QSet<int>>& data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
void LoadData(QString storageFolder, QString fileName, QHash<int, std::array<double, 22> > & data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genreLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
void LoadData(QString storageFolder, QString fileName, QHash<int, core::AuthorFavFandomStatsPtr>& data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
void LoadData(QString storageFolder, QString fileName, QVector<core::FicWeightPtr>& data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, vectorUnifier, storageFolder + QString("/") + fileName, data);
}

void LoadData(QString storageFolder, QString fileName, QHash<int, core::FicWeightPtr> & data)
{
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
void LoadData(QString storageFolder, QString fileName, QHash<int, QList<genre_stats::GenreBit>>& data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}

void LoadData(QString storageFolder, QString fileName, QHash<int, QString>& data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
void LoadData(QString storageFolder, QString fileName, QHash<uint32_t, genre_stats::ListMoodData> & data){
    qDebug() << "Loading:" << fileName;
    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
}
//void LoadData(QString storageFolder, QString fileName, QHash<int, double>& data){
//    qDebug() << "Loading:" << fileName;
//    loadMultiThreaded(genericLoader, hashUnifier, storageFolder + QString("/") + fileName, data);
//}
}

