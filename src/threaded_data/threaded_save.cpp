#include "threaded_data/threaded_save.h"

#include <QThread>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
struct DataKeeper{
    QFile data;
    QDataStream out;
    int fileCounter = 0;
};
namespace  thread_boost{
namespace  Impl{

auto fileWrapperVector = [](DataKeeper* obj, int threadCount, int taskSize, int chunkSize, QString nameBase, auto actualWork){
    obj->fileCounter = -1;
    for(int i = 0; i < taskSize; i ++)
    {
        if( i%chunkSize == 0)
        {
            obj->fileCounter++;
            obj->data.close();
            obj->data.setFileName(QString("%1_%2.txt").arg(nameBase).arg(QString::number(obj->fileCounter)));
            if(obj->data.open(QFile::WriteOnly | QFile::Truncate))
            {
                obj->out.setDevice(&obj->data);
                if(obj->fileCounter != threadCount)
                {
                    qDebug() << "writing task size of: " << chunkSize;
                    obj->out << chunkSize;
                }
                else
                {
                    qDebug() << "writing task size of: " << taskSize%threadCount;
                    obj->out << taskSize%threadCount;
                }
            }
            else
            {
                qDebug() << "breaking on error";
                break;
            }
        }
        actualWork(obj->out, i);
    }
};
auto fileWrapperHash = [](DataKeeper* obj,int threadCount, QString nameBase, auto container, auto actualWork){
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
            obj->data.setFileName(QString("%1_%2.txt").arg(nameBase).arg(QString::number(obj->fileCounter)));
            if(obj->data.open(QFile::WriteOnly | QFile::Truncate))
            {
                obj->out.setDevice(&obj->data);
                if(obj->fileCounter != threadCount)
                {
                    qDebug() << "writing task size of: " << chunkSize;
                    obj->out << (quint32)chunkSize;
                }
                else
                {
                    qDebug() << "writing task size of: " << taskSize%threadCount;
                    obj->out << (quint32)taskSize%threadCount;
                }
            }
        }
        actualWork(obj->out, it);
        it++;
    }
};


}
void SaveFicWeightCalcData(QString storageFolder,QVector<core::FicWeightPtr>& fics){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperVector(&keeper, threadCount, fics.size(),fics.size()/threadCount,storageFolder + "/fics",[&](auto& out,int i){
        fics[i]->Serialize(out);
    });
}
void SaveAuthorsData(QString storageFolder,QList<core::AuthorPtr>& authors){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperVector(&keeper, threadCount, authors.size(),authors.size()/threadCount, storageFolder +"/authors",[&](auto& out,int i){
        authors[i]->Serialize(out);});
}
void SaveFavouritesData(QString storageFolder, QHash<int, QSet<int>>& favourites){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount, storageFolder + "/fav", favourites, [&](auto& out, auto it){
        out << it.key();
        out << it.value();
    });
}
void SaveGenreDataForFavLists(QString storageFolder, QHash<int, std::array<double, 22> >& genreData){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount, storageFolder+ "/genre", genreData, [&](auto& out, auto it){
        out << it.key();
        for(auto value : it.value())
            out << value;
    });
}
void SaveFandomDataForFavLists(QString storageFolder, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount,storageFolder+ "/fandomstats", fandomLists, [&](auto& out, auto it){
        out << it.key();
        it.value()->Serialize(out);
    });
}
}
