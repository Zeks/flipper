#include "include/section.h"
#include <QDebug>

void core::Author::Log()
{
    qDebug() << "id: " << id;
    qDebug() << "name: " << name;
    qDebug() << "website: " << website;
    qDebug() << "urls: " << urls;
    qDebug() << "valid: " << isValid;
    qDebug() << "idStatus: " << static_cast<int>(idStatus);
    qDebug() << "ficCount: " << ficCount;
    qDebug() << "favCount: " << favCount;
    qDebug() << "lastUpdated: " << lastUpdated;
    qDebug() << "firstPublishedFic: " << firstPublishedFic;
}


