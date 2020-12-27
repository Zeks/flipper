#include "parsers/ffn/mobile_favparser.h"
#include "environment.h"
#include "pagegetter.h"
#include "timeutils.h"
#include <QCoreApplication>
namespace parsers{
namespace ffn{

MobileFavouritesFetcher::MobileFavouritesFetcher(QObject *parent):QObject(parent)
{
}

QSet<QString> MobileFavouritesFetcher::Execute()
{
    // first we need to create an m. link
    QString url = QString("https://m.fanfiction.net/u/%1//?a=fs").arg(userId);
    QString prototype = url;
    // we need to grab the initial page and figure out the exact amount of pages we need to parse

    WebPage page;
    TimedAction fetchAction("Author initial mobile page fetch", [&](){
        page = env::RequestPage(prototype.trimmed(),  ECacheMode::dont_use_cache);
    });
    fetchAction.run(false);

    //QString content;
    QRegularExpression rx("p[=](\\d+)['][>]Last[<][/]a[>]");
    auto match = rx.match(page.content);
    QSet<QString> urlResult;
    if(!match.hasMatch())
    {
        // failed to grab the last record, exiting with info from just basic page
        return urlResult;
    }
    int amountOfPagesToGrab = match.captured(1).toInt();

    // generating all of the urls that will need to be grabbed
    QStringList mobileUrls;
    // 26th page onwards can't be reached without m.
    mobileUrls.reserve(amountOfPagesToGrab);
    for(int i = pageToStartFrom; i <= amountOfPagesToGrab; i++)
        mobileUrls.push_back(prototype + "&s=0&cid=0&p=" + QString::number(i));

    //QList<QString> mobileStories;
    QRegularExpression rxStoryId("/s/(\\d+)/1");
    emit progress(0, amountOfPagesToGrab);
    int counter = 1;
    for(const auto& mobileUrl : mobileUrls)
    {
        WebPage page;
        TimedAction fetchAction("Author mobile page fetch", [&](){
            page = env::RequestPage(mobileUrl.trimmed(),  ECacheMode::dont_use_cache);
        });
        fetchAction.run(false);
        // need to fetch only story ids for now
        // this should be enough to create the rec list

        QRegularExpressionMatchIterator iterator = rxStoryId.globalMatch(page.content);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            QString word = match.captured(1);
            urlResult << word;
        }
        counter++;
        emit progress(counter, amountOfPagesToGrab);
        if(page.source != EPageSource::cache)
            QThread::msleep(timeout);
        QCoreApplication::processEvents();
    }
    return urlResult;
}

QSet<QString> MobileFavouritesFetcher::Execute(sql::Database db, ECacheMode cacheMode)
{
    // first we need to create an m. link
    QString url = QString("https://m.fanfiction.net/u/%1//?a=fs").arg(userId);
    QString prototype = url;
    // we need to grab the initial page and figure out the exact amount of pages we need to parse

    WebPage page;
    TimedAction fetchAction("Author initial mobile page fetch", [&](){
        page = env::RequestPage(prototype.trimmed(), db, cacheMode);
    });
    fetchAction.run(false);

    //QString content;
    QRegularExpression rx("p[=](\\d+)['][>]Last[<][/]a[>]");
    auto match = rx.match(page.content);
    QSet<QString> urlResult;
    if(!match.hasMatch())
    {
        // failed to grab the last record, exiting with info from just basic page
        return urlResult;
    }
    int amountOfPagesToGrab = match.captured(1).toInt();

    // generating all of the urls that will need to be grabbed
    QStringList mobileUrls;
    // 26th page onwards can't be reached without m.
    mobileUrls.reserve(amountOfPagesToGrab);
    for(int i = pageToStartFrom; i <= amountOfPagesToGrab; i++)
        mobileUrls.push_back(prototype + "&s=0&cid=0&p=" + QString::number(i));

    //QList<QString> mobileStories;
    QRegularExpression rxStoryId("/s/(\\d+)/1");
    emit progress(0, amountOfPagesToGrab);
    int counter = 1;
    for(const auto& mobileUrl : mobileUrls)
    {
        WebPage page;
        TimedAction fetchAction("Author mobile page fetch", [&](){
            page = env::RequestPage(mobileUrl.trimmed(),  db, cacheMode);
        });
        fetchAction.run(false);
        // need to fetch only story ids for now
        // this should be enough to create the rec list

        QRegularExpressionMatchIterator iterator = rxStoryId.globalMatch(page.content);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            QString word = match.captured(1);
            urlResult << word;
        }
        counter++;
        emit progress(counter, amountOfPagesToGrab);
        if(page.source != EPageSource::cache)
            QThread::msleep(timeout);
        QCoreApplication::processEvents();
    }
    return urlResult;
}



}}
