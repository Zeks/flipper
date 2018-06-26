#include"in_tag_accessor.h"

//void RecommendationsInfoAccessor::SetData(QString userToken, QSharedPointer<RecommendationsData> data)
//{
//    QWriteLocker locker(&lock);
//    recommendatonsData[userToken] = data;
//}

//QSharedPointer<RecommendationsData> RecommendationsInfoAccessor::GetData(QString userToken)
//{
//    QReadLocker locker(&lock);
//    return recommendatonsData[userToken];
//}

RecommendationsData* ThreadData::GetRecommendationData()
{
    thread_local RecommendationsData data;
    return &data;
}

UserData *ThreadData::GetUserData()
{
    thread_local UserData data;
    return &data;
}
