#include "discord/actions.h"
#include "discord/command_creators.h"
#include "discord/db_vendor.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/discord/users.h"
#include "grpc/grpc_source.h"
#include "timeutils.h"
#include <QUuid>
#include <QSettings>
namespace discord {

QSharedPointer<SendMessageCommand> ActionBase::Execute(QSharedPointer<TaskEnvironment> environment, Command command)
{
    action = SendMessageCommand::Create();
    action->originalMessage = command.originalMessage;
    action->user = command.user;
    if(command.type != Command::ECommandType::ct_timeout_active)
        command.user->initNewEasyQuery();
    return ExecuteImpl(environment, command);

}

template<typename T> QString GetHelpForCommandIfActive(){
    QString result;
    if(CommandState<T>::active)
        result = "\n" + CommandState<T>::help;
    return result;
}

QSharedPointer<SendMessageCommand> HelpAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command)
{
    QString helpString = "Basic commands:\n`!recs FFN_ID` to create recommendations";
    helpString +=  GetHelpForCommandIfActive<NextPageCommand>();
    helpString +=  GetHelpForCommandIfActive<PreviousPageCommand>();
    helpString +=  GetHelpForCommandIfActive<PageChangeCommand>();
    helpString +=  GetHelpForCommandIfActive<SetFandomCommand>();
    helpString +=  GetHelpForCommandIfActive<IgnoreFandomCommand>();
    helpString +=  GetHelpForCommandIfActive<IgnoreFicCommand>();
    helpString +=  GetHelpForCommandIfActive<DisplayHelpCommand>();

    //"\n!status to display the status of your recommentation list"
    //"\n!status fandom/fic X displays the status for fandom or a fic (liked, ignored)"
    action->text = helpString;
    return action;
}

QSet<QString> FetchUserFavourites(QString ffnId, QSharedPointer<SendMessageCommand> action){
    QSet<QString> userFavourites;

    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("pageCache");

    TimedAction linkGet("Link fetch", [&](){
        QString url = "https://www.fanfiction.net/u/" + ffnId;
        parsers::ffn::UserFavouritesParser parser;
        auto result = parser.FetchDesktopUserPage(ffnId,dbToken->db);
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
        parser.cacheDbToUse = dbToken->db;
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
    command.user->initNewRecsQuery();
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
    usersDbInterface->WriteUserFFNId(command.user->UserID(), command.ids.at(0));

    // instantiating working set for user
    An<Users> users;
    command.user->SetFicList(recList->ficData);
    command.user->SetFfnID(ffnId);
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
    filter.ignoreAlreadyTagged = false;
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
    auto fandomFilter = user->GetCurrentFandomFilter();
    if(fandomFilter.tokens.size() > 0){
        filter.fandom = fandomFilter.tokens.at(0).id;
        filter.includeCrossovers = fandomFilter.tokens.at(0).includeCrossovers;
    }
    if(fandomFilter.tokens.size() > 1){
        filter.secondFandom = fandomFilter.tokens.at(1).id;
        filter.includeCrossovers = true;
    }
    if(user->GetCurrentIgnoredFandoms().fandoms.size() > 0)
        filter.ignoreFandoms = true;
    UserData userData;

    auto ignoredFandoms =  user->GetCurrentIgnoredFandoms();
    for(auto& token: ignoredFandoms.tokens)
        userData.ignoredFandoms[token.id] = token.includeCrossovers;
    userData.allTaggedFics = user->GetIgnoredFics();
    QLOG_INFO() << "ignored fics: " << user->GetIgnoredFics();



    fics->clear();
    fics->reserve(size);
    source->userData = userData;
    source->FetchData(filter, fics);

}

QSharedPointer<SendMessageCommand> DisplayPageAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    QLOG_TRACE() << "Creating page results";
    auto page = command.ids.at(0);
    command.user->SetPage(page);
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->UpdateCurrentPage(command.user->UserID(), page);

    QVector<core::Fanfic> fics;
    QLOG_TRACE() << "Fetching fics";

    FetchFicsForList(environment->ficSource, command.user, 10, &fics);
    QLOG_TRACE() << "Fetched fics";
    SleepyDiscord::Embed embed;
    QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    environment->fandoms->db = dbToken->db;
    environment->fandoms->FetchFandomsForFics(&fics);
    embed.description = QString("Generated recs for user [%1](https://www.fanfiction.net/u/%1), page: %2\n\n").arg(command.user->FfnID()).arg(command.user->CurrentPage()).toStdString();

    QHash<int, int> positionToId;
    int i = 0;
    for(auto fic: fics)
    {
        positionToId[i+1] = fic.identity.id;
        i++;
        auto fandomsList=fic.fandoms;

        embed.description += QString("ID#: " + QString::number(i)).rightJustified(2, ' ').toStdString();
        embed.description += QString(" " + urlProto.arg(fic.title).arg(QString::number(fic.identity.web.GetPrimaryId()))+"\n").toStdString();

        if(fic.complete)
            embed.description += QString(" `Complete  `  ").toStdString();
        else
            embed.description += QString(" `Incomplete`").toStdString();
        embed.description += QString(" Length: `" + fic.wordCount + "`").toStdString();
        embed.description += QString(" Fandom: `" + fandomsList.join(" & ") + "`\n").rightJustified(20, ' ').toStdString();
        embed.description += "\n";
    }
    command.user->SetPositionsToIdsForCurrentPage(positionToId);
    action->embed = embed;
    QLOG_INFO() << "Created page results";
    return action;
}

QSharedPointer<SendMessageCommand> SetFandomAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    auto fandom = command.variantHash["fandom"].toString().trimmed();
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    environment->fandoms->db = dbToken->db;
    auto fandomId = environment->fandoms->GetIDForName(fandom);
    auto currentFilter = command.user->GetCurrentFandomFilter();
    An<interfaces::Users> usersDbInterface;
    if(command.variantHash.contains("reset"))
    {
        usersDbInterface->ResetFandomFilter(command.user->UserID());
        command.user->ResetFandomFilter();
        action->emptyAction = true;
        return action;
    }
    if(fandomId == -1)
    {
        action->text = "`" + fandom  + "`" + " is not a valid fandom";
        action->stopChain = true;
        return action;
    }
    if(currentFilter.fandoms.contains(fandomId))
    {
        currentFilter.RemoveFandom(fandomId);
        action->text = "Removing filtered fandom: " + fandom;
        usersDbInterface->UnfilterFandom(command.user->UserID(), fandomId);
    }
    else if(currentFilter.fandoms.size() == 2)
    {
        auto oldFandomId = currentFilter.tokens.last().id;
        action->text = "Replacing crossover fandom: " + environment->fandoms->GetNameForID(currentFilter.tokens.last().id) +    " with: " + fandom;
        usersDbInterface->UnfilterFandom(command.user->UserID(), oldFandomId);
        usersDbInterface->FilterFandom(command.user->UserID(), fandomId, command.variantHash["allow_crossovers"].toBool());
        currentFilter.RemoveFandom(oldFandomId);
        currentFilter.AddFandom(fandomId, command.variantHash["allow_crossovers"].toBool());
        return action;
    }
    else {
        usersDbInterface->FilterFandom(command.user->UserID(), fandomId, command.variantHash["allow_crossovers"].toBool());
        currentFilter.AddFandom(fandomId, command.variantHash["allow_crossovers"].toBool());
    }
    command.user->SetFandomFilter(currentFilter);
    action->emptyAction = true;
    return action;
}

QSharedPointer<SendMessageCommand> IgnoreFandomAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    auto fandom = command.variantHash["fandom"].toString().trimmed();
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    environment->fandoms->db = dbToken->db;
    auto fandomId = environment->fandoms->GetIDForName(fandom);

    auto withCrossovers = command.variantHash["with_crossovers"].toBool();
    An<interfaces::Users> usersDbInterface;
    if(command.variantHash.contains("reset"))
    {
        usersDbInterface->ResetFandomIgnores(command.user->UserID());
        command.user->ResetFandomIgnores();
        action->emptyAction = true;
        return action;
    }
    if(fandomId == -1)
    {
        action->text = "`" + fandom  + "`" + " is not a valid fandom";
        action->stopChain = true;
        return action;
    }

    auto currentIgnores = command.user->GetCurrentIgnoredFandoms();
    if(currentIgnores.fandoms.contains(fandomId)){
        usersDbInterface->UnignoreFandom(command.user->UserID(), fandomId);
        currentIgnores.RemoveFandom(fandomId);
    }
    else{
        usersDbInterface->IgnoreFandom(command.user->UserID(), fandomId, withCrossovers);
        currentIgnores.AddFandom(fandomId, withCrossovers);
    }
    command.user->SetIgnoredFandoms(currentIgnores);
    action->emptyAction = true;
    return action;
}

QSharedPointer<SendMessageCommand> IgnoreFicAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command command)
{
    auto ficIds = command.ids;
    if(command.variantHash.contains("everything"))
        ficIds = {1,2,3,4,5,6,7,8,9,10};

    auto ignoredFics = command.user->GetIgnoredFics();
    An<interfaces::Users> usersDbInterface;
    for(auto positionalId : ficIds){
        //need to get ffn id from positional id
        auto ficId = command.user->GetFicIDFromPositionId(positionalId);
        if(ficId != -1){
            if(!ignoredFics.contains(ficId))
            {
                ignoredFics.insert(ficId);
                usersDbInterface->TagFanfic(command.user->UserID(), "ignored",  ficId);
            }
            else{
                ignoredFics.remove(ficId);
                usersDbInterface->UnTagFanfic(command.user->UserID(), "ignored",  ficId);
            }
        }
    }
    command.user->SetIgnoredFics(ignoredFics);
    action->emptyAction = true;
    return action;
}

QSharedPointer<SendMessageCommand> TimeoutActiveAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command command)
{
    auto reason = command.variantHash["reason"].toString();
    auto seconds= command.ids.at(0);
    action->text = reason .arg(QString::number(seconds));
    return action;
}

QSharedPointer<SendMessageCommand> NoUserInformationAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command)
{
    action->text = "You need to call !recs FFN_ID first";
    return action;
}

QSharedPointer<ActionBase> GetAction(Command::ECommandType type)
{
    switch(type){
    case Command::ECommandType::ct_ignore_fics:
        return QSharedPointer<ActionBase>(new IgnoreFicAction());
    case Command::ECommandType::ct_ignore_fandoms:
        return QSharedPointer<ActionBase>(new IgnoreFandomAction());
    case Command::ECommandType::ct_set_fandoms:
        return QSharedPointer<ActionBase>(new SetFandomAction());
    case Command::ECommandType::ct_display_help:
        return QSharedPointer<ActionBase>(new HelpAction());
    case Command::ECommandType::ct_display_page:
        return QSharedPointer<ActionBase>(new DisplayPageAction());
    case Command::ECommandType::ct_timeout_active:
        return QSharedPointer<ActionBase>(new TimeoutActiveAction());
    case Command::ECommandType::ct_fill_recommendations:
        return QSharedPointer<ActionBase>(new RecsCreationAction());
    case Command::ECommandType::ct_no_user_ffn:
        return QSharedPointer<ActionBase>(new NoUserInformationAction());
    default:
        return QSharedPointer<ActionBase>();
    }
}



}


