#include "discord/actions.h"
#include "discord/command_creators.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/discord/users.h"
#include "grpc/grpc_source.h"
#include "timeutils.h"
#include <QUuid>
#include <QSettings>
namespace discord {

template<typename T> QString GetHelpForCommandIfActive(){
    QString result;
    if(CommandState<T>::active)
        result = "\n" + CommandState<T>::help;
    return result;
}

QSharedPointer<SendMessageCommand> HelpAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command)
{
    QString helpString = "!recs FFN_ID to create recommendations";
    helpString += "\n" + GetHelpForCommandIfActive<NextPageCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<PreviousPageCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<PageChangeCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<SetFandomCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<IgnoreFandomCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<IgnoreFandomWithCrossesCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<IgnoreFicCommand>();
    helpString += "\n" + GetHelpForCommandIfActive<DisplayHelpCommand>();

    //"\n!status to display the status of your recommentation list"
    //"\n!status fandom/fic X displays the status for fandom or a fic (liked, ignored)"
    action->text = helpString;
    return action;
}

QSet<QString> FetchUserFavourites(QString ffnId, QSharedPointer<SendMessageCommand> action){
    QSet<QString> userFavourites;

    QSqlDatabase pageCacheDb;
    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    auto uniqueId = QUuid::createUuid().toString();
    pageCacheDb = pageCacheInterface->InitDatabase("PageCache" + uniqueId);

    TimedAction linkGet("Link fetch", [&](){
        QString url = "https://www.fanfiction.net/u/" + ffnId;
        parsers::ffn::UserFavouritesParser parser;
        auto result = parser.FetchDesktopUserPage(ffnId);
        parsers::ffn::QuickParseResult quickResult;
        if(!result){
            action->errors.push_back("Could not load user page on FFN. Please send your FFN ID and this error to ficfliper@gmail.com if you want it fixed.");

            if(action->errors.size() == 0)
            {
                quickResult = parser.QuickParseAvailable();
                if(!quickResult.validFavouritesCount)
                    action->errors.push_back("Could not read favourites from your FFN page. Please send your FFN ID and this error to ficfliper@gmail.com if you want it fixed.");
            }
        }
        if(action->errors.size() == 0)
        {
            parser.FetchFavouritesFromDesktopPage();
            userFavourites = parser.result;

            if(!quickResult.canDoQuickParse)
            {
                parser.FetchFavouritesFromMobilePage();
                userFavourites = parser.result;
            }
        }
    });
    linkGet.run();
    pageCacheDb.removeDatabase("PageCache" + uniqueId);
    return userFavourites;
}

QSharedPointer<core::RecommendationList> CreateRecommendationParams(QString ffnId)
{
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
    list->userFFNId = ffnId.toInt();
    return list;
}

QSharedPointer<SendMessageCommand> RecsCreationAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    auto ffnId = QString::number(command.ids.at(0));
    core::RecommendationListFicData fics;
    QSharedPointer<core::RecommendationList> listParams;
    QString error;

    QSet<QString> userFavourites = FetchUserFavourites(ffnId, action);
    auto recList = CreateRecommendationParams(ffnId);

    QVector<core::Identity> pack;
    pack.resize(userFavourites.size());
    int i = 0;
    for(auto source: userFavourites)
    {
        pack[i].web.ffn = source.toInt();
        i++;
    }
    environment->ficSource->GetInternalIDsForFics(&pack);

    for(auto source: pack)
    {
        recList->ficData.sourceFics+=source.id;
        recList->ficData.fics+=source.web.ffn;
    }

    //QLOG_INFO() << "Getting fics";
    environment->ficSource->GetRecommendationListFromServer(*recList);
    //QLOG_INFO() << "Got fics";
    // refreshing the currently saved recommendation list params for user
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->DeleteUserList(command.user->UserID(), "");
    usersDbInterface->WriteUserList(command.user->UserID(), "", discord::elt_favourites, recList->minimumMatch, recList->maxUnmatchedPerMatch, recList->alwaysPickAt);

    // instantiating working set for user
    An<Users> users;
    command.user->SetFicList(recList->ficData);
    action->text = "Recommendation list has been created for FFN ID: " + QString::number(command.ids.at(0));
    return action;
}

void ActionChain::Push(QSharedPointer<SendMessageCommand> action)
{
    actions.push_back(action);
}

QSharedPointer<SendMessageCommand> ActionChain::Pop()
{
    auto action = actions.first();
    actions.pop_front();
    return action;
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

QSharedPointer<SendMessageCommand> DisplayPageAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    QLOG_INFO() << "Creating page results";
    auto page = command.ids.at(0);
    command.user->SetPage(page);
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->UpdateCurrentPage(command.user->UserID(), page);

    QVector<core::Fanfic> fics;
    QLOG_INFO() << "Fetching fics";

    FetchFicsForList(environment->ficSource, command.user, 10, &fics);
    QLOG_INFO() << "Fetched fics";
    SleepyDiscord::Embed embed;
    QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";

    environment->fandoms->FetchFandomsForFics(&fics);

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
    action->embed = embed;
    QLOG_INFO() << "Created page results";
    return action;
}

QSharedPointer<SendMessageCommand> ActionBase::Execute(QSharedPointer<TaskEnvironment> environment, Command command)
{
    action = SendMessageCommand::Create();
    action->originalMessage = command.originalMessage;
    action->user = command.user;
    return ExecuteImpl(environment, command);

}

}

