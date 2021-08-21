#include "parsers/ffn/discord/discord_mobile_favparser.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "discord/discord_pagegetter.h"
#include "discord/db_vendor.h"
#include "environment.h"
#include "timeutils.h"
#include <QCoreApplication>
namespace parsers{
namespace ffn{
namespace discord{
DiscordMobileFavouritesFetcher::DiscordMobileFavouritesFetcher(QObject *parent):QObject(parent)
{
}



QSet<int> DiscordMobileFavouritesFetcher::Execute(fetching::CacheStrategy cacheStrategy)
{
    // first we need to create an m. link
    QString url = QString("https://m.fanfiction.net/u/%1//?a=fs").arg(userId);
    QString prototype = url;
    // we need to grab the initial page and figure out the exact amount of pages we need to parse
    ::discord::PageManager::DBGetterFunc dbFetcher = []()->QSharedPointer<::discord::LockedDatabase>{
            return  An<::discord::DatabaseVendor>()->GetDatabase("pageCache");};
    ::discord::PageManager pageManager;
    pageManager.SetDatabaseGetter(dbFetcher);
    WebPage mobilePage;
    TimedAction fetchAction("Author initial mobile page fetch", [&](){
        mobilePage = pageManager.GetPage(prototype.trimmed(),  cacheStrategy);
        //qDebug() << mobilePage.content;

    });
    fetchAction.run(false);

    //QString content;
    QRegularExpression rx("p[=](\\d+)[\"][>]Last[<][/]a[>]");
    auto match = rx.match(mobilePage.content);
    QSet<int> urlResult;
    // failed to grab the last record, exiting with info from just basic page
    if(!match.hasMatch())
        return urlResult;

    url = "https://www.fanfiction.net/u/" + userId;
    WebPage desktopPage;
    TimedAction fetchDesktopAction("Fetching desktop page", [&](){
        desktopPage = pageManager.GetPage(url.trimmed(),  cacheStrategy);
    });
    fetchDesktopAction.run(false);

    parsers::ffn::UserFavouritesParser parser;
    parser.dektopPage = desktopPage;
    parser.FetchFavouritesFromDesktopPage();
    urlResult+=parser.GetResultAsIntSet();

    int amountOfPagesToGrab = match.captured(1).toInt();
    if(amountOfPagesToGrab <= 25)
        return urlResult;

    // generating all of the urls that will need to be grabbed
    QStringList mobileUrls;
    // 26th page onwards can't be reached without m.
    pageToStartFrom = 25;
    mobileUrls.reserve(amountOfPagesToGrab);
    for(int i = pageToStartFrom; i <= amountOfPagesToGrab; i++)
        mobileUrls.push_back(prototype + "&s=0&cid=0&p=" + QString::number(i));

    //QList<QString> mobileStories;
    QRegularExpression rxStoryId("/s/(\\d+)/1");
    emit progress(0, amountOfPagesToGrab);
    int counter = 1;
    for(const auto& mobileUrl : mobileUrls)
    {
        TimedAction fetchAction("Author mobile page fetch", [&](){
            mobilePage = pageManager.GetPage(mobileUrl.trimmed(),  cacheStrategy);
            //page = env::RequestPage(mobileUrl.trimmed(),  cacheMode);
        });
        fetchAction.run(false);
        // need to fetch only story ids for now
        // this should be enough to create the rec list
        QRegularExpressionMatchIterator iterator = rxStoryId.globalMatch(mobilePage.content);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            QString word = match.captured(1);
            urlResult << word.toInt();
        }
        counter++;
        emit progress(counter, amountOfPagesToGrab);
        if(mobilePage.source != EPageSource::cache)
            QThread::msleep(timeout);
        QCoreApplication::processEvents();
    }
    return urlResult;
}
}
}}
