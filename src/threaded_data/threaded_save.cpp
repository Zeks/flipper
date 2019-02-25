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
void SaveFavouritesData(QString storageFolder, QHash<int, Roaring>& favourites){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount, storageFolder + "/roafav", favourites, [&](auto& out, auto it){
        out << it.key();

        Roaring& r = it.value();
        size_t  expectedsize = r.getSizeInBytes();
        //qDebug() << "writing roaring of size: " << expectedsize;
        char *serializedbytes = new char [expectedsize];
        r.write(serializedbytes);
        QByteArray ba(QByteArray::fromRawData(serializedbytes, expectedsize));
        out << ba;
        delete[] serializedbytes;
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


void SaveData(QString storageFolder, QString fileName, QHash<int, Roaring>& favourites){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount, storageFolder + "/" + fileName, favourites, [&](auto& out, auto it){
        out << it.key();

        Roaring& r = it.value();
        size_t  expectedsize = r.getSizeInBytes();
        //qDebug() << "writing roaring of size: " << expectedsize;
        char *serializedbytes = new char [expectedsize];
        r.write(serializedbytes);
        QByteArray ba(QByteArray::fromRawData(serializedbytes, expectedsize));
        out << ba;
        delete[] serializedbytes;
    });
}
void SaveData(QString storageFolder, QString fileName, QHash<int, QSet<int>>& favourites){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount, storageFolder + "/" + fileName, favourites, [&](auto& out, auto it){
        out << it.key();
        out << it.value();
    });
}
void SaveData(QString storageFolder, QString fileName, QHash<int, std::array<double, 22> > &genreData){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount, storageFolder+ "/" + fileName, genreData, [&](auto& out, auto it){
        out << it.key();
        for(auto value : it.value())
            out << value;
    });
}
void SaveData(QString storageFolder, QString fileName, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount,storageFolder+ "/" + fileName, fandomLists, [&](auto& out, auto it){
        out << it.key();
        it.value()->Serialize(out);
    });
}
void SaveData(QString storageFolder, QString fileName, QVector<core::FicWeightPtr>& fics){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperVector(&keeper, threadCount, fics.size(),fics.size()/threadCount,storageFolder + "/" + fileName,[&](auto& out,int i){
        fics[i]->Serialize(out);
    });
}
void SaveData(QString storageFolder, QString fileName, QHash<int, core::FicWeightPtr> &fics){
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount,storageFolder+ "/" + fileName, fics, [&](auto& out, auto it){
        out << it.key();
        it.value()->Serialize(out);
    });
}

void SaveData(QString storageFolder, QString fileName, QHash<int, QList<genre_stats::GenreBit>>& fics)
{
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount,storageFolder+ "/" + fileName, fics, [&](auto& out, auto it){
        out << it.key();
        out << it.value();
    });
}

void SaveData(QString storageFolder, QString fileName, QHash<int, QString> &fics)
{
    DataKeeper keeper;
    int threadCount = QThread::idealThreadCount()-1;
    Impl::fileWrapperHash(&keeper, threadCount,storageFolder+ "/" + fileName, fics, [&](auto& out, auto it){
        out << it.key();
        out << it.value();
    });
}

}
