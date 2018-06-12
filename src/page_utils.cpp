#include "include/page_utils.h"
#include <QThread>

SplitJobs SplitJob(QString data, bool splitOnThreads)
{
    SplitJobs result;
    int threadCount;
    if(splitOnThreads)
        threadCount = QThread::idealThreadCount();
    else
        threadCount = 50;
    thread_local QRegExp rxStart("<div\\sclass=\'z-list\\sfavstories\'");
    int index = rxStart.indexIn(data);

    int captured = data.count(" favstories");
    result.favouriteStoryCountInWhole = captured;

    thread_local QRegExp rxAuthorStories("<div\\sclass=\'z-list\\smystories\'");
    index = rxAuthorStories.indexIn(data);
    int capturedAuthorStories = data.count(rxAuthorStories);
    result.authorStoryCountInWhole = capturedAuthorStories;

    int partSize = captured/(threadCount-1);
    //qDebug() << "In packs of "  << partSize;
    index = 0;

    if(partSize < 40)
        partSize = 40;

    QList<int> splitPositions;
    int counter = 0;
    do{
        index = rxStart.indexIn(data, index+1);
        if(counter%partSize == 0 && index != -1)
        {
            splitPositions.push_back(index);
        }
        counter++;
    }while(index != -1);


    result.parts.reserve(splitPositions.size());
    QStringList partSizes;
    for(int i = 0; i < splitPositions.size(); i++)
    {
        if(i != splitPositions.size()-1)
            result.parts.push_back({data.mid(splitPositions[i], splitPositions[i+1] - splitPositions[i]), i});
        else
            result.parts.push_back({data.mid(splitPositions[i], data.length() - splitPositions[i]),i});
        partSizes.push_back(QString::number(result.parts.last().data.length()));
    }
    return result;
}
