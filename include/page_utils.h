#pragma once
#include <QString>
#include <QVector>
struct SplitPart
{
    QString data;
    int partId;
};

struct SplitJobs
{
    QVector<SplitPart> parts;
    int favouriteStoryCountInWhole;
    int authorStoryCountInWhole;
    QString authorName;
};

SplitJobs SplitJob(QString data, bool splitOnThreads = true);
