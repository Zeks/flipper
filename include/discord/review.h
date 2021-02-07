#pragma once
#include <QString>
#include <QDateTime>

namespace discord{

struct ReviewFilter{
    enum ReviewType{
        rt_server = 0,
        rt_user = 0,
    };
    QString serverId;
    QString userId;
    QString ficId;
    QString ficUrl;
    QString reviewType; 
    bool allowGlobal = false;
    int reviewScoreCutoff = -5;
};


struct FicReview{
    //bool isGlobal = false;
    int score = 0; // neutral
    int reputation = 0; // neutral

    QString authorId;
    QString authorName;
    QString reviewId;
    QString serverId;
    QString reviewType; 
    QString text;
    QString reviewTitle;
    QString ficTitle;
    QString url;
    QString site;
    QString siteId;
    QDateTime published;
};


}
