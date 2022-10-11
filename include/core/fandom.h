#pragma once
#include "core/url.h"
#include "core/db_entity.h"
#include <QSharedPointer>
#include <QDate>

namespace core {

class Fandom;
typedef  QSharedPointer<Fandom> FandomPtr;



class Fandom
{
public:
    Fandom(){}
    Fandom(QString name){this->name = ConvertName(name);}
    static FandomPtr NewFandom() { return QSharedPointer<Fandom>(new Fandom);}
    QList<Url> GetUrls(){
        return urls;
    }
    void AddUrl(Url url){
        urls.append(url);
    }
    void SetName(QString name){this->name = ConvertName(name);}
    QString GetName() const;
    int id = -1;
    int idInRecentFandoms = -1;
    int ficCount = 0;
    double averageFavesTop3 = 0.0;

    QString section = "none";
    QString source = "ffn";
    QList<Url> urls;
    QDate dateOfCreation;
    QDate dateOfFirstFic;
    QDate dateOfLastFic;
    QDate lastUpdateDate;
    bool tracked = false;
    static QString ConvertName(QString name)
    {
        thread_local QHash<QString, QString> cache;
        name=name.trimmed();
        QString result;
        if(cache.contains(name))
            result = cache[name];
        else
        {
            QRegExp rx = QRegExp("(/(.|\\s){0,}[^\\x0000-\\x007F])|(/(.|\\s){0,}[?][?][?])");
            rx.setMinimal(true);
            int index = name.indexOf(rx);
            if(index != -1)
                cache[name] = name.left(index).trimmed();
            else
                cache[name] = name.trimmed();
            result = cache[name];
        }
        return result;
    }
private:
    QString name;

};

}
