#include "calc_data_holder.h"

#include <QThread>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

void CalcDataHolder::Clear(){
    fileCounter = -1;
    fics.clear();
    authors.clear();
    favourites.clear();
    genreData.clear();
    fandomLists.clear();
}
void CalcDataHolder::ResetFileCounter()
{
    fileCounter = -1;
}
void CalcDataHolder::SaveToFile(){}
void CalcDataHolder::LoadFromFile(){}
static auto fileWrapperVector = [](CalcDataHolder* obj, int threadCount, int taskSize, int chunkSize, QString nameBase, auto actualWork){
    obj->fileCounter = -1;
    for(int i = 0; i < taskSize; i ++)
    {
        if( i%chunkSize == 0)
        {
            obj->fileCounter++;
            obj->data.close();
            obj->data.setFileName(QString("TempData/%1_%2.txt").arg(nameBase).arg(QString::number(obj->fileCounter)));
            if(obj->data.open(QFile::WriteOnly | QFile::Truncate))
            {
                obj->out.setDevice(&obj->data);
                if(obj->fileCounter != threadCount)
                    obj->out << chunkSize;
                else
                    obj->out << taskSize%threadCount;
            }
            else
            {
                qDebug() << "breaking on error";
                break;
            }
        }
        actualWork(i);
    }
};
static auto fileWrapperHash = [](CalcDataHolder* obj,int threadCount, QString nameBase, auto container, auto actualWork){
    int counter = -1;
    int chunkSize = container.size()/threadCount;
    int taskSize = container.size();
    auto it = container.begin();
    obj->fileCounter = -1;
    while(it != container.end())
    {
        counter++;
        if( counter%chunkSize == 0)
        {
            obj->fileCounter++;
            obj->data.close();
            obj->data.setFileName(QString("TempData/%1_%2.txt").arg(nameBase).arg(QString::number(obj->fileCounter)));
            if(obj->data.open(QFile::WriteOnly | QFile::Truncate))
            {
                obj->out.setDevice(&obj->data);
                if(obj->fileCounter != threadCount)
                    obj->out << chunkSize;
                else
                    obj->out << taskSize%threadCount;
            }
        }
        actualWork(obj->out, it);
        it++;
    }
};

void CalcDataHolder::SaveFicsData(){
    int threadCount = QThread::idealThreadCount()-1;
    fileWrapperVector(this, threadCount, fics.size(),fics.size()/threadCount,"fics",[&](int i){
        fics[i]->Serialize(out);});
    fileWrapperVector(this, threadCount, authors.size(),authors.size()/threadCount,"authors",[&](int i){
        authors[i]->Serialize(out);});
    fileWrapperHash(this, threadCount, "fav", favourites, [&](auto& out, auto it){
        out << it.key();
        out << it.value();
    });
    fileWrapperHash(this, threadCount, "genre", genreData, [&](auto& out, auto it){
        out << it.key();
        for(auto value : it.value())
            out << value;
    });
    fileWrapperHash(this, threadCount, "fandomstats", fandomLists, [&](auto& out, auto it){
        out << it.key();
        it.value()->Serialize(out);
    });
    data.close();
}

void CalcDataHolder::LoadFicsData(){
    Clear();

    auto hashLoader = [](QString nameBase,
            auto resultCreator,
            int file,
            auto valueFetcher){
        auto resultHolder = resultCreator();
        QFile data(QString("%1_%2.txt").arg(nameBase).arg(QString::number(file)));
        if (data.open(QFile::ReadOnly)) {
            QDataStream in(&data);
            int size;
            in >> size;
            resultHolder.reserve(size);
            for(int i = 0; i < size; i++)
            {
                if(i%10000 == 0)
                    qDebug() << "processing fic: " << i;
                valueFetcher(resultHolder, in);
            }
        }
        else
            qDebug() << "Could not open file: " << QString("ficdata_%1.txt").arg(QString::number(file));
        return resultHolder;
    };

    auto loadMultiThreaded = [](auto loaderFunc,
            auto resultUnifier,
            QString nameBase,
            auto& destination){
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
        for(auto future: futures)
            resultUnifier(destination, future.result());
    };
    auto vectorUnifier = [](auto& dest, auto& source){dest+=source;};
    auto hashUnifier = [](auto& dest, auto& source){dest.unite(source);};


    auto ficFetchFunc = [](auto container, QDataStream& in){
        using PtrType = typename decltype(container)::value_type;
        using ValueType = typename decltype(container)::value_type::value_type;

        int key;
        in >> key;
        PtrType tmp{new ValueType()};
        tmp->Deserialize(in);
        container.push_back(tmp);
    };
    auto favouritesFetchFunc = [](auto container, QDataStream& in){
        int key;
        in >> key;
        typename decltype(container)::iterator::value_type values;
        in >> values;
        container[key] = values;
    };
    auto ficLoader = std::bind(hashLoader, std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      ficFetchFunc);
    auto authorLoader = std::bind(hashLoader, std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      ficFetchFunc);

    auto favouritesLoader = std::bind(hashLoader, std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      favouritesFetchFunc);
    auto genresFetchFunc = [](auto container, QDataStream& in){
        int key;
        in >> key;
        typename decltype(container)::iterator::value_type values;
        for(int i = 0; i < 22; i++)
            in >> values[i];
        container[key] = values;
    };
    auto genreLoader = std::bind(hashLoader, std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      genresFetchFunc);

    auto fandomListsFetchFunc =  [](auto container, QDataStream& in){
        int key;
        in >> key;
        using PtrType = typename decltype(container)::iterator::value_type;
        using ValueType = typename decltype(container)::iterator::value_type::value_type;
        PtrType value (new ValueType());
        value->Deserialize(in);
        container[key] = value;
    };
    auto fandomListsLoader = std::bind(hashLoader, std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      fandomListsFetchFunc);



    loadMultiThreaded(ficLoader,vectorUnifier, "fics", fics);
    loadMultiThreaded(ficLoader,vectorUnifier, "authors",authors);
    loadMultiThreaded(favouritesLoader, hashUnifier, "fav", favourites);
    loadMultiThreaded(genreLoader, hashUnifier, "genre",genreData);
    loadMultiThreaded(fandomListsLoader, hashUnifier, "fandomstats",fandomLists);


}
