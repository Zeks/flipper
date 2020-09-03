#include "discord/client.h"
#include "discord/discord_user.h"
#include "include/storyfilter.h"
#include "include/grpc/grpc_source.h"
#include "include/parsers/ffn/favparser_wrapper.h"

#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/discord/users.h"
#include "Interfaces/authors.h"
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"
#include "include/grpc/grpc_source.h"
#include "include/url_utils.h"
#include "include/timeutils.h"

#include "GlobalHeaders/SingletonHolder.h"
#include "logger/QsLog.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QDebug>
#include <QSettings>
#include <QtConcurrent>

void FetchFicsForList(QSharedPointer<FicSourceGRPC> source,
                      QSharedPointer<discord::User> user,
                      int size,
                      QVector<core::Fanfic>* fics);
ListData CreateListData(RecRequest request, QString userToken);

struct Task{
    std::string userId;
    std::string channelId;
    std::string url;
    QVector<int> ids;
    QStringList urls;
    QStringList recs;
};


void MyClientClass::onMessage(SleepyDiscord::Message message) {
    auto authorID = QString::fromStdString(message.author.ID.string());
    auto content = QString::fromStdString(message.content);


    if(message.author.ID == this->getID())
        return;

    QLOG_INFO() << content;

    if(!IsACommand(content))
        return;

    if(ProcessHelpRequest(message))
        return;

    bool knownUser = users.HasUser(authorID);
    bool needsGeneration = true;
    if(knownUser)
        needsGeneration = !users.GetUser(authorID)->HasActiveSet();

    needsGeneration = needsGeneration || message.startsWith("!recs");
    QLOG_INFO() << "known user: " << knownUser << " needsGeneration: " << needsGeneration;
    bool isNavigationCommand = false;
    isNavigationCommand = content.contains(QRegExp("^!(page|dig|dive|drop)"));
    isNavigationCommand = isNavigationCommand || content.contains(QRegExp("^!(next|ahoy|bitch|pls)"));
    isNavigationCommand = isNavigationCommand || content.contains(QRegExp("^!(prev|nope|shit|gah)"));

    if(!needsGeneration && knownUser)
    {
        auto user = users.GetUser(authorID);
        if(content.contains(QRegExp("^!(page|dig|dive|drop)")))
        {
            auto match = pageExp.match(QString::fromStdString(message.content));
            if(match.hasMatch())
                user->SetPage(match.capturedTexts().at(1).toInt());

            SendLongList(user,  message.channelID);
            return;
        }
        if(content.contains(QRegExp("^!(next|ahoy|bitch|pls)")))
        {
            SendLongList(user,  message.channelID, 1);
            return;
        }
        if(content.contains(QRegExp("^!(prev|nope|shit|gah)")))
        {
            SendLongList(user,  message.channelID, -1);
            return;
        }
    }

    if(isNavigationCommand)
    {
        QLOG_INFO() << "caught navigation command but activeset is not present";
    }

    if(!isNavigationCommand && !content.contains(QRegExp("^!recs")))
        return;

    auto matchRecs = recsExp.match(QString::fromStdString(message.content));
    QStringList captures = matchRecs.capturedTexts();
    QString ffnId;
    if(matchRecs.hasMatch())
        ffnId = matchRecs.capturedTexts().at(1);
    else return;

    if(matchRecs.capturedLength() < content.length())
    {
        sendMessage(message.channelID, "Correct format for recs is either `!recs USER_ID` or `!recs USER_ID MIN_MATCHES RATIO ALWAYS_PICK`\nUse `!help recs` to get info on what these do.");
        return;
    }


    if(needsGeneration && !knownUser)
    {
        //sendMessage(message.channelID, "needs generation");
        QSharedPointer<discord::User> user(new discord::User(authorID, ffnId, QString::fromStdString(message.author.username)));
        users.userInterface->WriteUser(user);
        users.LoadUser(authorID);
    }


    auto user = users.GetUser(authorID);
    if(!user)
    {
        QLOG_INFO()  << "null user";
        return;
    }
    //sendMessage(message.channelID, QString("secs since last query: " + QString::number(user->secsSinceLastsRecQuery())).toStdString());

    QSettings settings("settings_discord.ini", QSettings::IniFormat);
    if(user->secsSinceLastsRecQuery() < 60 && settings.value("Main/enableLimits", true).toBool())
    {
        sendMessage(message.channelID, QString("Please wait %1 more seconds.").arg(60 - user->secsSinceLastsRecQuery()).toStdString());
        return;
    }
    RecRequest rec;
    rec.ffnID = ffnId;
    rec.userID = authorID;
    rec.channelId = message.channelID;
    rec.requestArguments = captures;


    if(watcher.isRunning() )
    {
        sendMessage(message.channelID, "Your request has been sent to queue, please wait.");
        recRequests.push_back(rec);
        return;
    }
    sendMessage(message.channelID, "Creating recommendation list, please wait.");
    user->initNewRecsQuery();
    watcher.setFuture(QtConcurrent::run(CreateListData, rec, userDbInterface->GetUserToken()));

    QLOG_INFO() << "Received message:";
    QLOG_INFO() << "When: " << QString::fromStdString(message.timestamp);
    QLOG_INFO() << "Channel: " << QString::fromStdString(message.channelID);
    QLOG_INFO() << "User: " << QString::fromStdString(message.author.ID);
    QLOG_INFO() << "Message type" <<  message.type;
    QLOG_INFO() << "Text: " << QString::fromStdString(message.content);
}

bool MyClientClass::ProcessHelpRequest(SleepyDiscord::Message &message)
{
    bool isHelp = message.startsWith("!help") || message.startsWith("!halp");
    if(!isHelp)
        return false;

    if(message.startsWith("!help recs"))
    {
        sendMessage(message.channelID, CreateRecsHelpMessage().toStdString());
        return true;
    }
    sendMessage(message.channelID, "!recs FFN_ID for recommendations, !next,!prev,!page X for navigation"
                                   "\n!status to display the status of your recommentation list"
                                   "\n!fandom for single fandom searches, repeat the same command to reset"
                                   "\n!ignfandom to permanently ignore a fandom, repeat to unignore, fandom must be entered exactly the same as the bot shows it"
                                   "\n!xignfandom to permanently ignore a fandom **with crossovers**, repeat to unignore"
                                   "\n!ignfandom without an argument will display a list of current ignores"
                                   "\n!tagfanfic X Y will hide fic(s) from further display where X is an arbitrary tag and Y is (multiple) # next to a fic separated by whitespaces");
    return true;
}

bool MyClientClass::IsACommand(QString content)
{
    if(content.trimmed().isEmpty())
        return false;
    content = content.trimmed();
    if(content.left(1) != "!")
        return false;

    static auto commandFiller = [](){
        QStringList commands;
    commands << "!recs" << "!fandom" << "!ignfandom" << "!xignfandom" << "!tagfanfic" << "!help" << "!status" << "!help";
    commands << "!page" << "!dig" << "!dive" << "!drop";
    commands << "!prev" << "!nope" << "!shit" << "!gah";
    commands << "!next" << "!ahoy" << "!bitch" << "!pl";
    return commands;
    };
    static QStringList commands = commandFiller();

    int pos = content.indexOf(" ");
    if(pos != -1)
        content = content.mid(0, pos);
    if(!commands.contains(content))
        return false;
    return true;
}

QString MyClientClass::CreateRecsHelpMessage()
{
    QString result;
    result+="`!recs USER_ID` will create a recommendation list based on user's FFN favourites.\n`USER_ID` is numeric part of a link like that: `https://www.fanfiction.net/u/4507073`\n\n";
    result+="`!recs USER_ID MIN_MATCHES RATIO ALWAYS_PICK` allows more detailed control over list creation.\n`1.USER_ID` is the same as above, the other arguments are as follows:\n";
    result+="`2.MIN_MATCHES` how much fics at minimum must match  between your favourites and user's for them to be considered\n";
    result+="`3.RATIO` how much fics not in your fav list can be in user's favourites per one fic that is.\nThe smaller this number, the less recommendations will be found but quality will be better.\n";
    result+="`4.ALWAYS_PICK` If a user has at least X fics matched with your favourites, ignore the ratio\n";
    return result;
}

void MyClientClass::OnFutureFinished()
{
    QLOG_INFO() << "Reached future slot";
    auto result = watcher.future().result();
    auto user = users.GetUser(result.userId);
    QLOG_INFO() << "created list of size: " << result.fics.fics.size() << " with matchcounts: " << result.fics.matchCounts.size();
    if(!result.error.isEmpty())
    {
        SleepyDiscord::Embed embed;
        sendMessage(result.channelID, result.error.toStdString(), embed);
        return;
    }
    users.userInterface->DeleteUserList(result.userId, "");
    users.userInterface->WriteUserList(result.userId, "", discord::elt_favourites, result.listParams->minimumMatch, result.listParams->maxUnmatchedPerMatch, result.listParams->alwaysPickAt);
    user->SetFicList(result.fics);

    SendLongList(user,  result.channelID);

    if(recRequests.size() > 0)
    {
        auto first = recRequests.first();
        recRequests.pop_front();
        auto user = users.GetUser(first.userID);
        user->initNewRecsQuery();
        watcher.setFuture(QtConcurrent::run(CreateListData, first, userDbInterface->GetUserToken()));
    }
}


MyClientClass::MyClientClass(const std::string token, const char numOfThreads, QObject *obj):QObject(obj),
    SleepyDiscord::DiscordClient(token, numOfThreads)
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    qRegisterMetaType<ListData>("ListData");

    QObject::connect(&watcher, &QFutureWatcher<ListData>::finished,
                     this, &MyClientClass::OnFutureFinished);
    RxInit();
    //UsersInit();
}

MyClientClass::MyClientClass(QObject *obj):QObject(obj), SleepyDiscord::DiscordClient()
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    qRegisterMetaType<ListData>("ListData");

    QObject::connect(&watcher, &QFutureWatcher<ListData>::finished,
                     this, &MyClientClass::OnFutureFinished);
    RxInit();
    //UsersInit();
}

void MyClientClass::RxInit()
{
    recsExp.setPattern("^[!]recs\\s(\\d{1,15})(\\s(\\d{1,3})\\s(\\d{1,3})\\s(\\d{1,4})){0,}");
    pageExp.setPattern("^[!]page\\s(\\d{1,5})");
}

void MyClientClass::UsersInit()
{
    users.InitInterface(userDbInterface->GetDatabase());
}

void MyClientClass::SendDetailedList(QSharedPointer<discord::User> user,
                                SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID,
                                int advance)
{
    if(advance)
    {
        user->AdvancePage(advance);
        users.userInterface->UpdateCurrentPage(user->UserID(), user->CurrentPage());
    }

    QVector<core::Fanfic> fics;
    FetchFicsForList(ficSource, user, 4, &fics);
    SleepyDiscord::Embed embed;
    embed.description = QString("Generated recs for user [%1](https://www.fanfiction.net/u/%1), page: %2\n\n").arg(user->FfnID()).arg(user->CurrentPage()).toStdString();
    QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";
    for(auto fic: fics)
    {
        QStringList fandomsList;
        for(auto fandom : fic.fandomIds)
            if(fandom != -1)
                fandomsList+=fandoms->GetNameForID(fandom);

        embed.description += QString("ID#: " + QString::number(fic.identity.web.GetPrimaryId())).rightJustified(10, ' ').toStdString();
        embed.description += QString(" " + urlProto.arg(fic.title).arg(QString::number(fic.identity.web.GetPrimaryId()))+"\n").toStdString();

        if(fic.complete)
            embed.description += QString(" `Complete`  ").toStdString();
        else
            embed.description += QString(" `Incomplete`").toStdString();
        embed.description += QString(" Length: `" + fic.wordCount + "`").toStdString();
        embed.description += QString(" Fandom: `" + fandomsList.join(" & ") + "`\n").rightJustified(20, ' ').toStdString();
        embed.description += "\n";
        embed.description += QString(fic.summary + "\n").toStdString();
        embed.description += "\n";
    }
    sendMessage(channelID, "", embed);
}

void MyClientClass::SendLongList(QSharedPointer<discord::User> user, SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, int advance)
{
    if(advance)
    {
        user->AdvancePage(advance);
        users.userInterface->UpdateCurrentPage(user->UserID(), user->CurrentPage());
    }


    QVector<core::Fanfic> fics;
    QLOG_INFO() << "Fetching fics";

    FetchFicsForList(ficSource, user, 10, &fics);
    QLOG_INFO() << "Fetched fics";
    SleepyDiscord::Embed embed;
    embed.description = QString("Generated recs for user [%1](https://www.fanfiction.net/u/%1), page: %2\n\n").arg(user->FfnID()).arg(user->CurrentPage()).toStdString();
    QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";

    fandoms->FetchFandomsForFics(&fics);

    for(auto fic: fics)
    {
        auto fandomsList=fic.fandoms;

        embed.description += QString("ID#: " + QString::number(fic.GetIdInDatabase())).rightJustified(10, ' ').toStdString();
        embed.description += QString(" " + urlProto.arg(fic.title).arg(QString::number(fic.identity.web.GetPrimaryId()))+"\n").toStdString();

        if(fic.complete)
            embed.description += QString(" `Complete  `  ").toStdString();
        else
            embed.description += QString(" `Incomplete`").toStdString();
        embed.description += QString(" Length: `" + fic.wordCount + "`").toStdString();
        embed.description += QString(" Fandom: `" + fandomsList.join(" & ") + "`\n").rightJustified(20, ' ').toStdString();
        embed.description += "\n";
    }
    QLOG_INFO() << "Generated results";
    sendMessage(channelID, "", embed);
    QLOG_INFO() << "Sent message";
}




void FetchFicsForList(QSharedPointer<FicSourceGRPC> source,
                      QSharedPointer<discord::User> user,
                      int size,
                      QVector<core::Fanfic>* fics)
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentPage();
    filter.ignoreAlreadyTagged = true;
    filter.showOriginsInLists = false;
    filter.recordLimit = size;
    filter.sortMode = core::StoryFilter::sm_metascore;
    filter.reviewBias = core::StoryFilter::bias_none;
    filter.mode = core::StoryFilter::filtering_in_fics;
    //filter.mode = core::StoryFilter::filtering_in_recommendations;
    filter.slashFilter.excludeSlash = true;
    filter.slashFilter.includeSlash = false;
    filter.slashFilter.slashFilterLevel = 1;
    filter.slashFilter.slashFilterEnabled = true;
    auto userFics = user->FicList();
    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics[i]))
            continue;
        filter.recsHash[userFics->fics[i]] = userFics->matchCounts[i];
    }

    fics->clear();
    fics->reserve(size);
    source->FetchData(filter, fics);

}


inline QString CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    std::string result = server_address_proto.arg(ip).arg(port).toStdString();
    return QString::fromStdString(result);
}


ListData CreateListData(RecRequest request, QString userToken){
    Task task;
    ListData actualResult;

    QSqlDatabase pageCacheDb;
    //TimedAction dbInit("DB Init", [&](){
        QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
        pageCacheDb = pageCacheInterface->InitDatabase("PageCache");
    //});
    //dbInit.run();


    QSet<QString> userFavourites;

    TimedAction linkGet("Link fetch", [&](){
        QString url = "https://www.fanfiction.net/u/" + request.ffnID;
        task.url = url.toStdString();
        parsers::ffn::UserFavouritesParser parser;
        auto result = parser.FetchDesktopUserPage(request.ffnID);
        if(!result)
        {
            actualResult.error = "Could not load user page on FFN. Please send your FFN ID and this error to ficfliper@gmail.com if you want it fixed.";
            return;
        }

        auto quickResult = parser.QuickParseAvailable();
        if(!quickResult.validFavouritesCount)
        {
            actualResult.error = "Could not read favourites from your FFN page. Please send your FFN ID and this error to ficfliper@gmail.com if you want it fixed.";
            return;
        }

            parser.FetchFavouritesFromDesktopPage();
            userFavourites = parser.result;

        if(!quickResult.canDoQuickParse)
        {
            parser.FetchFavouritesFromMobilePage();
            userFavourites = parser.result;
        }
    });
    linkGet.run();

    if(!actualResult.error.isEmpty())
    {
        actualResult.userId = request.userID;
        actualResult.channelID = request.channelId;
        return actualResult;
    }

//    TimedAction dbClose("DB CLose", [&](){
    pageCacheDb.removeDatabase("PageCache");
    //});
    //dbClose.run();

    for(auto fic: userFavourites)
        task.ids += fic.toInt();

    QSettings settings("settings/settings_discord.ini", QSettings::IniFormat);

    auto ip = settings.value("Login/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Login/serverPort", "3055").toString();

    QLOG_INFO() << "will connect to grpc via: " << ip << " "  << port;
    static QSharedPointer<FicSourceGRPC> ficSource;
    ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), userToken,  160));



    QSharedPointer<core::RecommendationList> list(new core::RecommendationList);
    list->minimumMatch = 6;
    list->maxUnmatchedPerMatch = 50;
    list->alwaysPickAt = 9999;
    list->isAutomatic = true;
    list->useWeighting = true;
    list->useMoodAdjustment = true;
    list->name = "Recommendations";
    list->assignLikedToSources = true;
    list->name = "generic";
    list->userFFNId = request.ffnID.toInt();


    QVector<core::Identity> pack;
    pack.resize(task.ids.size());
    int i = 0;
    for(auto source: task.ids)
    {
        pack[i].web.ffn = source;
        i++;
    }
    ficSource->GetInternalIDsForFics(&pack);
    for(auto source: pack)
    {
        list->ficData.sourceFics+=source.id;
        list->ficData.fics+=source.web.ffn;
    }

    //QLOG_INFO() << "Getting fics";
    ficSource->GetRecommendationListFromServer(*list);
    //QLOG_INFO() << "Got fics";




    actualResult.userId = request.userID;
    actualResult.channelID = request.channelId;
    actualResult.fics = list->ficData;
    actualResult.listParams = list;
    return actualResult;
}

