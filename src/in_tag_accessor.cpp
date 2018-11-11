/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
