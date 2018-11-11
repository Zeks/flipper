#include "threaded_data/threaded_load.h"
#include "threaded_data/common_traits.h"

#include <QThread>
#include <QDebug>
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
    using ContainerType = typename std::remove_reference<decltype(container)>::type;
    using ContainerValueType =  typename ContainerValueTypeGetter<ContainerType>::type;

    int key;
    if constexpr(is_hash<ContainerType>::value)
        in >> key;

    if constexpr(is_serializable_pointer<ContainerValueType>())
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
    QList<QFuture<typename std::remove_reference<decltype(destination)>::type>> futures;
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
    qDebug() << "finished unification";
};

auto vectorUnifier = [](auto& dest, auto& source){dest+=source;};
auto hashUnifier = [](auto& dest, auto& source){dest.unite(source);};
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

}

