#pragma once
#include <QString>
#include <QVector>
#include "include/pagetask.h"
namespace interfaces{
class PageTask;
}
namespace page_utils{
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

// creates the task and subtasks to load more authors from urls found in the database
PageTaskPtr CreatePageTaskFromUrls(QSharedPointer<interfaces::PageTask>,
                                   QDateTime currentDateTime,
                                   QStringList urls,
                                   QString taskComment,
                                   int subTaskSize,
                                   int subTaskRetries,
                                   ECacheMode cacheMode,
                                   bool allowCacheRefresh);
}
