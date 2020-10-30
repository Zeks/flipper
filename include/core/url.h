#pragma once
#include <QString>
namespace core {
class Url
{
public:
    Url(QString url, QString source, QString type = "default"){
        this->url = url;
        this->source = source;
        this->type = type;
    }
    QString GetUrl(){return url;}
    QString GetSource(){return source;}
    QString GetType(){return type;}
    void SetType(QString value) {type = value;}
private:
    QString url; // actual url
    QString source; // website of origin
    QString type; // type of url on website
};

}
