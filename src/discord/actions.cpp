#include "discord/actions.h"
#include "discord/command_creators.h"
#include "discord/discord_init.h"
#include "discord/db_vendor.h"
#include "sql/discord/discord_queries.h"
#include "discord/discord_server.h"
#include "discord/fetch_filters.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/discord/users.h"
#include "grpc/grpc_source.h"
#include "timeutils.h"
#include <QUuid>
#include <QRegExp>
#include <QSettings>
namespace discord {

QSharedPointer<SendMessageCommand> ActionBase::Execute(QSharedPointer<TaskEnvironment> environment, Command command)
{
    action = SendMessageCommand::Create();
    action->originalMessage = command.originalMessage;
    action->user = command.user;
    if(command.type != Command::ECommandType::ct_timeout_active)
        command.user->initNewEasyQuery();
    environment->ficSource->SetUserToken(command.user->GetUuid());
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
    helpString +=  GetHelpForCommandIfActive<RngCommand>();
    //"\n!status to display the status of your recommentation list"
    //"\n!status fandom/fic X displays the status for fandom or a fic (liked, ignored)"
    action->text = helpString;
    return action;
}

QSet<QString> FetchUserFavourites(QString ffnId, QSharedPointer<SendMessageCommand> action, bool useCache = true){
    QSet<QString> userFavourites;

    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("pageCache");

    TimedAction linkGet("Link fetch", [&](){
        QString url = "https://www.fanfiction.net/u/" + ffnId;
        parsers::ffn::UserFavouritesParser parser;
        auto result = parser.FetchDesktopUserPage(ffnId,dbToken->db, useCache);
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

//QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
//params->minimumMatch = 1;
//params->maxUnmatchedPerMatch = 50;
//params->alwaysPickAt = 9999;
//params->isAutomatic = true;
//params->useWeighting = true;
//params->useMoodAdjustment = true;
//params->name = "Recommendations";
//params->assignLikedToSources = true;
//params->userFFNId = env->interfaces.recs->GetUserProfile();

QSharedPointer<core::RecommendationList> CreateRecommendationParams(QString ffnId)
{
    QSharedPointer<core::RecommendationList> list(new core::RecommendationList);
    list->minimumMatch = 1;
    list->maxUnmatchedPerMatch = 50;
    list->isAutomatic = true;
    list->useWeighting = true;
    list->alwaysPickAt = 9999;
    list->useMoodAdjustment = true;
    list->name = "Recommendations";
    list->assignLikedToSources = true;
    list->userFFNId = ffnId.toInt();
    return list;
}

QSharedPointer<SendMessageCommand> RecsCreationAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    command.user->initNewRecsQuery();
    auto ffnId = QString::number(command.ids.at(0));
    bool refreshing = command.variantHash.contains("refresh");
    core::RecommendationListFicData fics;
    QSharedPointer<core::RecommendationList> listParams;
    QString error;

    QSet<QString> userFavourites = FetchUserFavourites(ffnId, action, refreshing);
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
    QMap<int, int> scoreStatus; // maps maptch count to fic count with this match
    QMap<int, QSet<int>> matchFicToScore; // maps maptch count to fic count with this match
    int count = 0;
    if(!recList->ficData.matchCounts.size())
    {
        command.user->SetFfnID(ffnId);
        action->text = "Couldn't create recommendations. Recommendations server is not available? Ping the author: zekses#3495";
        action->stopChain = true;
        return action;

    }
    for(int i = 0; i < recList->ficData.fics.size(); i++){
        if(recList->ficData.matchCounts.at(i) > 1)
        {
            matchFicToScore[recList->ficData.matchCounts.at(i)].insert(recList->ficData.fics.at(i));
            count++;
        }
    }
    int perfectRange = count*0.05; // 5% of the list with score > 1
    int goodRange = count*.15; // 15% of the list with score > 1
    int perfectCutoff = 0, goodCutoff = 0;
    auto keys = matchFicToScore.keys();
    std::sort(keys.begin(), keys.end());
    std::reverse(keys.begin(), keys.end());
    int currentFront = 0, previousFront = 0, previousKey = 0;
    QSet<int> perfectRngFics, goodRngFics;
    for(auto key : keys){
        previousKey = key;
        previousFront = currentFront;
        currentFront += matchFicToScore[key].size();
        if(currentFront > perfectRange && perfectCutoff == 0)
            perfectCutoff = key;
        else if(currentFront  < perfectRange){
            perfectRngFics += matchFicToScore[key];
        }
        goodRngFics += matchFicToScore[key];
        if(currentFront > goodRange && goodCutoff == 0)
        {
            goodCutoff = key;
            break;
        }
    }
    if(goodCutoff == 0)
        goodCutoff=2;

    QLOG_INFO() << "Total fic count: " << count << " Perfect Fics: " << perfectRngFics.size() << " Good Fics: " << goodRngFics.size();


    command.user->SetFfnID(ffnId);
    //command.user->SetPerfectRngFics(perfectRngFics);
    //command.user->SetGoodRngFics(goodRngFics);
    command.user->SetPerfectRngScoreCutoff(perfectCutoff);
    command.user->SetGoodRngScoreCutoff(goodCutoff);
    if(!refreshing)
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

static std::string CreateMention(const std::string& string){
    return "<@" + string + ">";
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

    FetchFicsForDisplayPageCommand(environment->ficSource, command.user, 10, &fics);
    auto userFics = command.user->FicList();
    for(auto& fic : fics)
        fic.score = userFics->ficToScore[fic.identity.id];
    QLOG_TRACE() << "Fetched fics";
    SleepyDiscord::Embed embed;
    QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";
    QString authorUrlProto = "[%1](https://www.fanfiction.net/u/%2)";
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    environment->fandoms->db = dbToken->db;
    environment->fandoms->FetchFandomsForFics(&fics);
    action->text = QString::fromStdString(CreateMention(command.originalMessage.author.ID.string()) + ", here are the results:");
    embed.description = QString("Generated recs for user [%1](https://www.fanfiction.net/u/%1), page: %2\n\n").arg(command.user->FfnID()).arg(command.user->CurrentPage()).toStdString();

    QHash<int, int> positionToId;
    int i = 0;
    for(auto fic: fics)
    {
        positionToId[i+1] = fic.identity.id;
        i++;
        auto fandomsList=fic.fandoms;
        embed.description += QString("ID: " + QString::number(i)).rightJustified(2, ' ').toStdString();
        embed.description += QString(" " + urlProto.arg(fic.title).arg(QString::number(fic.identity.web.GetPrimaryId()))+"\n").toStdString();
        embed.description += QString("Fandom: `" + fandomsList.join(" & ").replace("'", "\\'") + "`").rightJustified(20, ' ').toStdString();
        //embed.description += " by: "  + QString(" " + authorUrlProto.arg(fic.author).arg(QString::number(fic.author_id))+"\n").toStdString();
        embed.description += QString("\nStatus:  ").toHtmlEscaped().toStdString();
        if(fic.complete)
            embed.description += QString(" `Complete  `  ").toStdString();
        else
            embed.description += QString(" `Incomplete`").toStdString();
        embed.description += QString(" Length: `" + fic.wordCount + "`").toStdString();
        embed.description += QString(" Score: `" + QString::number(fic.score) + "`").toStdString();
        QString genre = fic.statistics.realGenreString.split(",").join("/").replace(QRegExp("(c|b|p)#"),"").replace("#", "");
        if(genre.isEmpty())
            genre = fic.genreString;
        embed.description += QString("\nGenre: `" + genre + "`").toStdString();
        embed.description += "\n\n";
        auto temp =  QString::fromStdString(embed.description);
        temp = temp.replace("````", "```");
        embed.description = temp.toStdString();
    }

    auto filter = command.user->GetCurrentFandomFilter();
    if(filter.fandoms.size() > 0){
        embed.description += "\nDisplayed recommendations are for fandom filter:\n";
        for(auto fandom: filter.fandoms)
        {
            if(fandom != -1)
                embed.description += ( " - " + environment->fandoms->GetNameForID(fandom) + "\n").toStdString();
        }
    }

    command.user->SetPositionsToIdsForCurrentPage(positionToId);
    action->embed = embed;
    QLOG_INFO() << "Created page results";
    return action;
}


QSharedPointer<SendMessageCommand> DisplayRngAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command command)
{
    auto quality = command.variantHash["quality"].toString().trimmed();
    QVector<core::Fanfic> fics;


    QSet<int> filteringFicSet;
    int scoreCutoff = 1;
    if(quality == "best")    {
        scoreCutoff= command.user->GetPerfectRngScoreCutoff();
    }
    if(quality == "good"){
        scoreCutoff= command.user->GetGoodRngScoreCutoff();
    }

    QLOG_TRACE() << "Fetching fics for rng";
    FetchFicsForDisplayRngCommand(3, environment->ficSource, command.user, &fics, scoreCutoff);
    auto userFics = command.user->FicList();
    for(auto& fic : fics)
        fic.score = userFics->ficToScore[fic.identity.id];

    QLOG_TRACE() << "Fetched fics for rng";

    // fetching fandoms for selected fics
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    environment->fandoms->db = dbToken->db;
    environment->fandoms->FetchFandomsForFics(&fics);

    SleepyDiscord::Embed embed;
    QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";


    QHash<int, int> positionToId;
    int i = 0;
    for(auto fic: fics)
    {
        positionToId[i+1] = fic.identity.id;
        i++;
        auto fandomsList=fic.fandoms;

        embed.description += QString("ID: " + QString::number(i)).rightJustified(2, ' ').toStdString();
        embed.description += QString(" " + urlProto.arg(fic.title.replace("'", "\'")).arg(QString::number(fic.identity.web.GetPrimaryId()))+"\n").toStdString();
        embed.description += QString("Fandom: `" + fandomsList.join(" & ").replace("'", "\'") + "`").rightJustified(20, ' ').toStdString();

        embed.description += "\nStatus:  ";
        if(fic.complete)
            embed.description += QString(" `Complete  `  ").toStdString();
        else
            embed.description += QString(" `Incomplete`").toStdString();
        embed.description += QString(" Length: `" + fic.wordCount + "`").toStdString();
        embed.description += QString(" Score: `" + QString::number(fic.score) + "`").toStdString();
        QString genre = fic.statistics.realGenreString.split(",").join("/").replace(QRegExp("(c|b|p)#"),"").replace("#", "");
        if(genre.isEmpty())
            genre = fic.genreString;
        genre= genre.replace("`", "");
        embed.description += QString("\nGenre: `" + genre + "`").toStdString();
        embed.description += (QString("\n```") + fic.summary + QString("```")).toStdString();
        embed.description += "\n";
        auto temp =  QString::fromStdString(embed.description);
        temp = temp.replace("````", "```");
        //temp = temp.replace("'", "\'");
        embed.description = temp.toStdString();
    }
    auto filter = command.user->GetCurrentFandomFilter();
    if(filter.fandoms.size() > 0){
        embed.description += "\nDisplayed recommendations are for fandom filter:\n";
        for(auto fandom: filter.fandoms)
        {
            if(fandom != -1)
                embed.description += ( " - " + environment->fandoms->GetNameForID(fandom) + "\n").toStdString();
        }
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
    auto properNameForFandom = environment->fandoms->GetNameForID(fandomId);

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
    auto ignoreFandom = [&](){
        usersDbInterface->IgnoreFandom(command.user->UserID(), fandomId, withCrossovers);
        currentIgnores.AddFandom(fandomId, withCrossovers);
        action->text = "Adding fandom: " + properNameForFandom + " to ignores";
        if(withCrossovers)
            action->text += ".Will also exclude crossovers from now on.";
        else
            action->text += ".";
    };

    if(currentIgnores.fandoms.contains(fandomId)){
        auto token = currentIgnores.GetToken(fandomId);
        if(!token.includeCrossovers && withCrossovers)
        {
            currentIgnores.RemoveFandom(fandomId);
            ignoreFandom();
        }
        else{
            usersDbInterface->UnignoreFandom(command.user->UserID(), fandomId);
            currentIgnores.RemoveFandom(fandomId);
            action->text = "Removing fandom: " + properNameForFandom + " from ignores";
        }
    }
    else{
        ignoreFandom();

    }
    command.user->SetIgnoredFandoms(currentIgnores);
    //action->emptyAction = true;
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

QSharedPointer<SendMessageCommand> ChangePrefixAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command command)
{
    if(command.variantHash.contains("prefix")){
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
        command.server->SetCommandPrefix(command.variantHash["prefix"].toString().trimmed());
        auto regex = GetSimpleCommandIdentifierPrefixless();
        command.server->SetQuickCommandIdentifier(std::regex((command.server->GetCommandPrefix().trimmed() + regex).toStdString()));
        database::discord_queries::WriteServerPrefix(dbToken->db, command.server->GetServerId(), command.server->GetCommandPrefix().trimmed());
        action->text = "Prefix has been changed";
    }
    else
        action->text = "Prefix wasn't changed because of an error";
    return action;
}
QSharedPointer<SendMessageCommand> InsufficientPermissionsAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command)
{
    action->text = "You don't have required permissions on the server to run this command.";
    return action;
}
QSharedPointer<SendMessageCommand> NullAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command)
{
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
    case Command::ECommandType::ct_display_rng:
        return QSharedPointer<ActionBase>(new DisplayRngAction());
        case Command::ECommandType::ct_change_server_prefix:
            return QSharedPointer<ActionBase>(new ChangePrefixAction());
        case Command::ECommandType::ct_insufficient_permissions:
            return QSharedPointer<ActionBase>(new InsufficientPermissionsAction());
    default:
        return QSharedPointer<ActionBase>(new NullAction());
    }
}








}




