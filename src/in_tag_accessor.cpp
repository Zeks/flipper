#include"in_tag_accessor.h"

void RecommendationsInfoAccessor::SetData(QString userToken, QSharedPointer<RecommendationsData> data)
{
    QWriteLocker locker(&lock);
    recommendatonsData[userToken] = data;
}

QSharedPointer<RecommendationsData> RecommendationsInfoAccessor::GetData(QString userToken)
{
    QReadLocker locker(&lock);
    return recommendatonsData[userToken];
}
