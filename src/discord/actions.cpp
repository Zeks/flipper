#include "discord/actions.h"
#include "discord/command_generators.h"
#include "discord/discord_init.h"
#include "discord/help_generator.h"
#include "discord/db_vendor.h"
#include "discord/client_v2.h"
#include "sql/discord/discord_queries.h"
#include "discord/discord_server.h"
#include "discord/discord_message_token.h"
#include "discord/fetch_filters.h"
#include "discord/favourites_fetching.h"
#include "discord/type_strings.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "include/qstring_from_stringview.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/discord/users.h"
#include "grpc/grpc_source.h"
#include "third_party/nanobench/nanobench.h"
#include "timeutils.h"
#include <QUuid>
#include <QRegExp>
#include <QSettings>
namespace discord {

QSharedPointer<SendMessageCommand> ActionBase::Execute(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    action = SendMessageCommand::Create();
    action->originalMessageToken = command.originalMessageToken;
    action->user = command.user;
    action->originalCommandType = command.type;
    if(command.type != ECommandType::ct_timeout_active)
        command.user->initNewEasyQuery();
    environment->ficSource->SetUserToken(command.user->GetUuid());
    return ExecuteImpl(environment, std::move(command));

}

template<typename T> QString GetHelpForCommandIfActive(){
    QString result;
    if(CommandState<T>::active)
        result = "\n" + QString::fromStdString(std::string(TypeStringHolder<T>::help));
    return result;
}

QSharedPointer<SendMessageCommand> HelpAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    QString helpString;
    helpString +=  GetHelpForCommandIfActive<RecsCreationCommand>();
    helpString +=  GetHelpForCommandIfActive<NextPageCommand>();
    helpString +=  GetHelpForCommandIfActive<PreviousPageCommand>();
    helpString +=  GetHelpForCommandIfActive<PageChangeCommand>();
    helpString +=  GetHelpForCommandIfActive<SetFandomCommand>();
    helpString +=  GetHelpForCommandIfActive<IgnoreFandomCommand>();
    helpString +=  GetHelpForCommandIfActive<IgnoreFicCommand>();
    helpString +=  GetHelpForCommandIfActive<DisplayHelpCommand>();
    helpString +=  GetHelpForCommandIfActive<RngCommand>();
    helpString +=  GetHelpForCommandIfActive<FilterLikedAuthorsCommand>();
    helpString +=  GetHelpForCommandIfActive<ShowFreshRecsCommand>();
    helpString +=  GetHelpForCommandIfActive<ShowCompletedCommand>();
    helpString +=  GetHelpForCommandIfActive<HideDeadCommand>();
    helpString +=  GetHelpForCommandIfActive<SimilarFicsCommand>();
    helpString +=  GetHelpForCommandIfActive<ResetFiltersCommand>();
    helpString +=  GetHelpForCommandIfActive<ChangeServerPrefixCommand>();
    helpString +=  GetHelpForCommandIfActive<ChangePermittedChannelCommand>();
    helpString +=  GetHelpForCommandIfActive<PurgeCommand>();


    //"\n!status to display the status of your recommentation list"
    //"\n!status fandom/fic X displays the status for fandom or a fic (liked, ignored)"
    action->text = helpString.arg(QSV(command.server->GetCommandPrefix()));
    return action;
}


QSharedPointer<SendMessageCommand> GeneralHelpAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    auto embed = GetHelpPage(command.ids.at(0), command.server->GetCommandPrefix());
    action->embed = embed;
    command.user->SetCurrentHelpPage(command.ids.at(0));
    if(command.targetMessage.string().length() > 0)
        action->targetMessage = command.targetMessage;
    action->reactionsToAdd.push_back(QStringLiteral("%f0%9f%91%88"));
    action->reactionsToAdd.push_back(QStringLiteral("%f0%9f%91%89"));

    return action;
}


QSharedPointer<core::RecommendationList> CreateRecommendationParams(QString ffnId)
{
    QSharedPointer<core::RecommendationList> list(new core::RecommendationList);
    list->minimumMatch = 1;
    list->maxUnmatchedPerMatch = 50;
    list->isAutomatic = true;
    list->useWeighting = true;
    list->alwaysPickAt = 9999;
    list->useMoodAdjustment = true;
    list->name = QStringLiteral("Recommendations");
    list->assignLikedToSources = true;
    list->userFFNId = ffnId.toInt();
    return list;
}


QSharedPointer<core::RecommendationList> CreateSimilarFicParams()
{
    QSharedPointer<core::RecommendationList> list(new core::RecommendationList);
    list->minimumMatch = 1;
    list->maxUnmatchedPerMatch = 5000;
    list->isAutomatic = false;
    list->useWeighting = false;
    list->alwaysPickAt = 9999;
    list->useMoodAdjustment = false;
    list->name = QStringLiteral("Recommendations");
    list->assignLikedToSources = true;
    list->userFFNId = -1;
    return list;
}

QSharedPointer<core::RecommendationList> FillUserRecommendationsFromFavourites(QString ffnId, const QSet<QString>& userFavourites, QSharedPointer<TaskEnvironment> environment, const Command& command){
    auto recList = CreateRecommendationParams(ffnId);
    recList->ignoreBreakdowns= true;

    QVector<core::Identity> pack;
    pack.resize(userFavourites.size());
    int i = 0;
    for(const auto& source: userFavourites)
    {
        pack[i].web.ffn = source.toInt();
        i++;
    }

    environment->ficSource->ClearUserData();
    environment->ficSource->GetInternalIDsForFics(&pack);
    UserData userData;

    auto ignoredFandoms =  command.user->GetCurrentIgnoredFandoms();
    for(auto& token: ignoredFandoms.tokens)
        recList->ignoredFandoms.insert(token.id);

    for(const auto& source: std::as_const(pack))
    {
        recList->ficData->sourceFics+=source.id;
        recList->ficData->fics+=source.web.ffn;
    }

    environment->ficSource->userData = userData;
    environment->ficSource->GetRecommendationListFromServer(recList);
    //QLOG_INFO() << "Got fics";
    // refreshing the currently saved recommendation list params for user
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->DeleteUserList(command.user->UserID(), QStringLiteral(""));
    usersDbInterface->WriteUserList(command.user->UserID(), QStringLiteral(""), discord::elt_favourites, recList->minimumMatch, recList->maxUnmatchedPerMatch, recList->alwaysPickAt);
    usersDbInterface->WriteUserFFNId(command.user->UserID(), command.ids.at(0));

    // instantiating working set for user
    An<Users> users;
    command.user->SetFicList(recList->ficData);
    //QMap<int, int> scoreStatus; // maps maptch count to fic count with this match
    QMap<int, QSet<int>> matchFicToScore; // maps maptch count to fic count with this match
    int count = 0;
    if(recList->ficData->matchCounts.isEmpty())
        return recList;

    for(int i = 0; i < recList->ficData->fics.size(); i++){
        if(recList->ficData->matchCounts.at(i) > 1)
        {
            matchFicToScore[recList->ficData->matchCounts.at(i)].insert(recList->ficData->fics.at(i));
            count++;
        }
    }
    int perfectRange = count*0.05; // 5% of the list with score > 1
    int goodRange = count*0.15; // 15% of the list with score > 1
    int perfectCutoff = 0, goodCutoff = 0;
    auto keys = matchFicToScore.keys();
    std::sort(keys.begin(), keys.end());
    std::reverse(keys.begin(), keys.end());
    int currentFront = 0;
    QSet<int> perfectRngFics, goodRngFics;
    for(auto key : keys){
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

    QLOG_INFO() << QStringLiteral("Total fic count: ") << count << QStringLiteral(" Perfect Fics: ") << perfectRngFics.size() << QStringLiteral(" Good Fics: ") << goodRngFics.size();

    command.user->SetFfnID(ffnId);
    //command.user->SetPerfectRngFics(perfectRngFics);
    //command.user->SetGoodRngFics(goodRngFics);
    command.user->SetPerfectRngScoreCutoff(perfectCutoff);
    command.user->SetGoodRngScoreCutoff(goodCutoff);
    environment->ficSource->ClearUserData();
    return recList;
}






QSharedPointer<SendMessageCommand> MobileRecsCreationAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    //Client::allowMessages = false;
//    ankerl::nanobench::Bench().minEpochIterations(6).run(
//                [&](){
    command.user->initNewRecsQuery();

    auto ffnId = QString::number(command.ids.at(0));
    bool refreshing = command.variantHash.contains(QStringLiteral("refresh"));
    auto largeListToken =  command.user->GetLargeListToken();
    An<interfaces::Users> usersDbInterface;
    if(!refreshing){
        if(largeListToken.date == QDate::currentDate() && largeListToken.counter == 2)
        {
            action->text = "Only two profile reparses per day are allowed for lists over 500 fics.";
            action->stopChain = true;
            return action;
        }

        if(largeListToken.date == QDate::currentDate()){
            largeListToken.counter++;
            command.user->SetLargeListToken(largeListToken);
        }
        else{
            largeListToken.date = QDate::currentDate();
            largeListToken.counter = 1;
            command.user->SetLargeListToken(largeListToken);
        }
        usersDbInterface->WriteLargeListReparseToken(command.user->UserID(), largeListToken);
    }

    QSharedPointer<core::RecommendationList> listParams;
    //QString error;

    FavouritesFetchResult userFavourites = FetchMobileFavourites(ffnId, refreshing ? ECacheMode::use_only_cache : ECacheMode::dont_use_cache);
    // here, we check that we were able to fetch favourites at all
    if(!userFavourites.hasFavourites || userFavourites.errors.size() > 0)
    {
        action->text = userFavourites.errors.join(QStringLiteral("\n"));
        action->stopChain = true;
        return action;
    }
    // here, we check that we were able to fetch all favourites with desktop link and reschedule the task otherwise
    if(userFavourites.requiresFullParse)
    {
        action->text = QStringLiteral("Your favourite list is bigger than 500 favourites, sending it to secondary parser. You will be pinged when the recommendations are ready.");
        return action;
    }
    bool wasAutomatic = command.user->GetForcedMinMatch() == 0;
    auto recList = FillUserRecommendationsFromFavourites(ffnId, userFavourites.links, environment, command);
    if(wasAutomatic && !recList->isAutomatic)
    {
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
        environment->fandoms->db = dbToken->db;

        usersDbInterface->WriteForcedListParams(command.user->UserID(), recList->minimumMatch,recList->maxUnmatchedPerMatch);
    }

    //qDebug() << "after filling";

    if(recList->ficData->matchCounts.isEmpty())
    {
        command.user->SetFfnID(ffnId);
        action->text = QStringLiteral("Couldn't create recommendations. Recommendations server is not available or you don't have any favourites on your ffn page. If it isn't the latter case, you can ping the author: zekses#3495");
        action->stopChain = true;
        return action;
    }

    if(!refreshing)
        action->text = QStringLiteral("Recommendation list has been created for FFN ID: ") + QString::number(command.ids.at(0));
    environment->ficSource->ClearUserData();
    command.user->SetRngBustScheduled(true);
//    });
//    Client::allowMessages = true;
    return action;
}

static std::string CreateMention(const std::string& string){
    return "<@" + string + ">";
}

QSharedPointer<SendMessageCommand> DesktopRecsCreationAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    command.user->initNewRecsQuery();

    QString ffnId;
    bool isId = true;
    if(!command.variantHash.contains(QStringLiteral("url"))){
        ffnId = QString::number(command.ids.at(0));
        isId = true;
    }
    else{
        isId = false;
        ffnId = command.variantHash[QStringLiteral("url")].toString();
    }

    bool refreshing = command.variantHash.contains(QStringLiteral("refresh"));
    bool keepPage = command.variantHash.contains(QStringLiteral("keep_page"));
    QSharedPointer<core::RecommendationList> listParams;
    //QString error;

    FavouritesFetchResult userFavourites = TryFetchingDesktopFavourites(ffnId, refreshing ? ECacheMode::use_only_cache : ECacheMode::dont_use_cache, isId);
    ffnId = userFavourites.ffnId;
    command.ids.clear();
    command.ids.push_back(userFavourites.ffnId.toUInt());


    // here, we check that we were able to fetch favourites at all
    if(!userFavourites.hasFavourites || userFavourites.errors.size() > 0)
    {
        action->text = userFavourites.errors.join(QStringLiteral("\n"));
        action->stopChain = true;
        return action;
    }
    // here, we check that we were able to fetch all favourites with desktop link and reschedule the task otherwise
    if(userFavourites.requiresFullParse)
    {
        action->stopChain = true;
        action->text = QString::fromStdString(CreateMention(command.user->UserID().toStdString()) + " Your favourite list is bigger than 500 favourites, sending it to secondary parser. You will be pinged when the recommendations are ready.");
        Command newRecsCommand = NewCommand(command.server, command.originalMessageToken, ct_create_recs_from_mobile_page);
        newRecsCommand.variantHash = command.variantHash;
        newRecsCommand.ids = command.ids;
        newRecsCommand.user = command.user;
        Command displayRecs = NewCommand(command.server, command.originalMessageToken, ct_display_page);
        displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
        displayRecs.user = command.user;
        if(!keepPage)
            displayRecs.ids.push_back(0);
        else
            displayRecs.ids.push_back(command.user->CurrentRecommendationsPage());
        CommandChain chain;
        chain.user = command.user;
        chain.Push(std::move(newRecsCommand));
        chain.Push(std::move(displayRecs));
        chain.hasFullParseCommand = true;
        action->commandsToReemit.push_back(std::move(chain));
        return action;
    }
    bool wasAutomatic = command.user->GetForcedMinMatch() == 0;
    auto recList = FillUserRecommendationsFromFavourites(ffnId, userFavourites.links, environment,command);
    if(wasAutomatic && !recList->isAutomatic)
    {
        command.user->SetForcedMinMatch(recList->minimumMatch);
        command.user->SetForcedRatio(recList->maxUnmatchedPerMatch);
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
        environment->fandoms->db = dbToken->db;
        An<interfaces::Users> usersDbInterface;
        usersDbInterface->WriteForcedListParams(command.user->UserID(), recList->minimumMatch,recList->maxUnmatchedPerMatch);
    }


    if(recList->ficData->matchCounts.isEmpty())
    {
        command.user->SetFfnID(ffnId);
        action->text = QString::fromStdString(CreateMention(command.user->UserID().toStdString()) + " Couldn't create recommendations. Recommendations server is not available or you don't have any favourites on your ffn page. If it isn't the latter case, you can ping the author: zekses#3495");
        action->stopChain = true;
        return action;
    }

    if(!refreshing)
        action->text = QStringLiteral("Recommendation list has been created for FFN ID: ") + QString::number(command.ids.at(0));
    command.user->SetRngBustScheduled(true);
    environment->ficSource->ClearUserData();
    QThread::msleep(50);
    return action;
}



void ActionChain::Push(QSharedPointer<SendMessageCommand> action)
{
    actions.push_back(action);
}

QSharedPointer<SendMessageCommand> ActionChain::Pop()
{
    auto action = std::as_const(actions.first());
    actions.pop_front();
    return action;
}


auto ExtractAge(QDateTime toDate){
    int years = 0;
    int months = 0;
    int days = 0;
    QDate beginDate= toDate.date();
    QDate endDate= QDateTime::currentDateTimeUtc().date();
    if(beginDate.daysTo(endDate) >= 0)
    {
        years=endDate.year()-beginDate.year();
        if((months=endDate.month()-beginDate.month())<0)
        {
            years--;
            months+=12;
        }
        if((days=endDate.day()-beginDate.day())<0)
        {
            if(--months < 0)
            {
                years--;
                months+=12;
            }
            days+=beginDate.daysInMonth();
        }
      }

    QStringList dates;
    if(years > 0)
        dates += QString::number(years) + QStringLiteral("y");
    if(months > 0)
        dates += QString::number(months) + QStringLiteral("m");
    if(days > 0)
        dates += QString::number(days) + QStringLiteral("d");
    return dates;
}

void  FillListEmbedForFicAsFields(SleepyDiscord::Embed& embed, const core::Fanfic& fic, int i, bool addNewlines = true){
    QString urlProto = QStringLiteral("[%1](https://www.fanfiction.net/s/%2)");

    auto fandomsList=fic.fandoms;
    SleepyDiscord::EmbedField field;
    field.isInline = true;
    field.name = QString(QStringLiteral("Fandom: `") + fandomsList.join(QStringLiteral(" & ")).replace(QStringLiteral("'"), QStringLiteral("\\'")) + QStringLiteral("`")).rightJustified(20, ' ').toStdString();
    field.value += QString(QStringLiteral("ID: ") + QString::number(i)).rightJustified(2, ' ').toStdString();
    field.value += QString(QStringLiteral(" ") + urlProto.arg(fic.title, QString::number(fic.identity.web.GetPrimaryId()))+QStringLiteral("\n")).toStdString();

    field.value += QString(QStringLiteral("Length: `") + fic.wordCount + QStringLiteral("`")).toStdString();
    field.value += QString(QStringLiteral("\nScore: `") + QString::number(fic.score) + "`").toStdString();
    //field.value += QString("n").toHtmlEscaped().toStdString();
    QString genre = fic.statistics.realGenreString.split(QStringLiteral(",")).join(QStringLiteral("/"))
            .replace(QRegExp(QStringLiteral("#c#")),QStringLiteral("+"))
            .replace(QRegExp(QStringLiteral("#p#")),QStringLiteral("="))
            .replace(QRegExp(QStringLiteral("#b#")),QStringLiteral("~"));
    if(genre.isEmpty())
        genre = fic.genreString;

    field.value  += QString(QStringLiteral("\nGenre: `") + genre + "`").toStdString();
    if(fic.complete)
        field.value  += QStringLiteral("\nComplete").rightJustified(12).toStdString();
    else
    {
        QDateTime date = fic.updated.isValid() && fic.updated.date().year() != 1970 ? fic.updated : fic.published;
        auto dates = ExtractAge(date);
        if(dates.size())
            field.value  += (QStringLiteral("\nIncomplete: `") + dates.join(QStringLiteral(" ")) + QStringLiteral("`")).rightJustified(12).toStdString();
        else
            field.value  += QStringLiteral("\nIncomplete:`").rightJustified(12).toStdString();
    }

    if(addNewlines)
        field.value  += "\n\n";
    auto temp =  QString::fromStdString(field.value);
    temp = temp.replace(QStringLiteral("````"), QStringLiteral("```"));
    field.value  = temp.toStdString();
    embed.fields.push_back(field);
}


void  FillListEmbedForFic(SleepyDiscord::Embed& embed, const core::Fanfic& fic, int i, bool addNewlines = true){
    QString urlProto = QStringLiteral("[%1](https://www.fanfiction.net/s/%2)");
    auto fandomsList=fic.fandoms;
    embed.description += QString(QStringLiteral("ID: ") + QString::number(i)).rightJustified(2, ' ').toStdString();
    embed.description += QString(QStringLiteral(" ") + urlProto.arg(fic.title, QString::number(fic.identity.web.GetPrimaryId()))+QStringLiteral("\n")).toStdString();
    embed.description += QString(QStringLiteral("Fandom: `") +
                                 fandomsList.join(QStringLiteral(" & ")).replace(QStringLiteral("'"), QStringLiteral("\\'"))
                                 + QStringLiteral("`")).rightJustified(20, ' ').toStdString();
    //embed.description += " by: "  + QString(" " + authorUrlProto.arg(fic.author).arg(QString::number(fic.author_id))+"\n").toStdString();
    embed.description += QString(QStringLiteral("\nLength: `") + fic.wordCount + QStringLiteral("`")).toStdString();
    embed.description += QString(QStringLiteral(" Score: `") + QString::number(fic.score) + QStringLiteral("`")).toStdString();
    embed.description += QStringLiteral(" Status:  ").toHtmlEscaped().toStdString();
    if(fic.complete)
        embed.description += QStringLiteral(" `Complete`  ").rightJustified(12).toStdString();
    else
    {
        QDateTime date = fic.updated.isValid() && fic.updated.date().year() != 1970 ? fic.updated : fic.published;
        auto dates = ExtractAge(date);
        if(dates.size())
            embed.description += (QStringLiteral(" `Incomplete: ") + dates.join(QStringLiteral(" ")) + QStringLiteral("`")).rightJustified(12).toStdString();
        else
            embed.description += QStringLiteral(" `Incomplete`").rightJustified(12).toStdString();
    }
    QString genre = fic.statistics.realGenreString.split(QStringLiteral(",")).join(QStringLiteral("/")).
            replace(QRegExp(QStringLiteral("#c#")),QStringLiteral("+")).
            replace(QRegExp(QStringLiteral("#p#")),QStringLiteral("=")).
            replace(QRegExp(QStringLiteral("#b#")),QStringLiteral("~"));
    if(genre.isEmpty())
        genre = fic.genreString;

    embed.description += QString(QStringLiteral("\nGenre: `") + genre + "`").toStdString();
    if(addNewlines)
        embed.description += "\n\n";
    auto temp =  QString::fromStdString(embed.description);
    temp = temp.replace(QStringLiteral("````"), QStringLiteral("```"));
    embed.description = temp.toStdString();
}



void  FillDetailedEmbedForFic(SleepyDiscord::Embed& embed, core::Fanfic& fic, int i, bool asFields){
    if(asFields)
        FillListEmbedForFicAsFields(embed, fic, i);
    else{
        FillListEmbedForFic(embed, fic, i, false);
        embed.description += (QString(QStringLiteral("\n```")) + fic.summary + QString(QStringLiteral("```"))).toStdString();
        embed.description += "\n";
        auto temp =  QString::fromStdString(embed.description);
        temp = temp.replace(QStringLiteral("````"), QStringLiteral("```"));
        //temp = temp.replace("'", "\'");
        embed.description = temp.toStdString();
    }
}




void  FillActiveFilterPartInEmbed(SleepyDiscord::Embed& embed, QSharedPointer<TaskEnvironment> environment, const Command& command){
    auto filter = command.user->GetCurrentFandomFilter();

    QString result;
    if(command.user->GetSimilarFicsId() != 0)
        result += QString(QStringLiteral("\nDisplaying similarity list for fic: %1.")).arg(command.user->GetSimilarFicsId());
    if(filter.fandoms.size() > 0){
        result += QStringLiteral("\nDisplayed recommendations are for fandom filter:\n");
        for(auto fandom: std::as_const(filter.fandoms))
        {
            if(fandom != -1)
                result += ( QStringLiteral(" - ") + environment->fandoms->GetNameForID(fandom) + QStringLiteral("\n"));
        }
    }
    static constexpr uint16_t oneThousandWords = 1000;
    auto wordcountFilter = command.user->GetWordcountFilter();
    if(wordcountFilter.firstLimit !=0 && wordcountFilter.secondLimit/oneThousandWords == 99999999)
        result += QString(QStringLiteral("\nShowing fics with > %1k words.")).arg(QString::number(wordcountFilter.firstLimit/oneThousandWords));
    if(wordcountFilter.firstLimit == 0 && wordcountFilter.secondLimit != 0)
        result += QString(QStringLiteral("\nShowing fics with < %1k words.")).arg(QString::number(wordcountFilter.secondLimit/oneThousandWords));
    if(wordcountFilter.firstLimit != 0 && wordcountFilter.secondLimit != 0  && wordcountFilter.secondLimit/1000 != 99999999)
        result += QString(QStringLiteral("\nShowing fics between %1k and %2k words.")).arg(QString::number(wordcountFilter.firstLimit/oneThousandWords),QString::number(wordcountFilter.secondLimit/oneThousandWords));


    if(command.user->GetLastPageType() == ct_display_rng)
        result += QString(QStringLiteral("\nRolling in range: %1.")).arg(command.user->GetLastUsedRoll());
    if(command.user->GetUseLikedAuthorsOnly())
        result += QStringLiteral("\nLiked authors filter is active.");
    if(command.user->GetSortFreshFirst())
        result += QStringLiteral("\nFresh recommendations sorting is active.");
    if(command.user->GetShowCompleteOnly())
        result += QStringLiteral("\nOnly showing fics that are complete.");
    if(command.user->GetHideDead())
        result += QStringLiteral("\nOnly showing fics that are not dead.");
    if(!result.isEmpty())
    {
        QString temp = QStringLiteral("\nTo disable any active filters, repeat the command that activates them,\nor issue %2xfilter to remove them all.");
        temp = temp.arg(QSV(command.server->GetCommandPrefix()));
        result += temp;
    }

    embed.description += result.toStdString();
}

void  FillActiveFilterPartInEmbedAsField(SleepyDiscord::Embed& embed, QSharedPointer<TaskEnvironment> environment, const Command& command){
    auto filter = command.user->GetCurrentFandomFilter();
    SleepyDiscord::EmbedField field;
    field.isInline = false;
    field.name = "Active filters:";
    if(filter.fandoms.size() > 0){
        field.value += "\nDisplayed recommendations are for fandom filter:\n";
        for(auto fandom: std::as_const(filter.fandoms))
        {
            if(fandom != -1)
                field.value += ( " - " + environment->fandoms->GetNameForID(fandom) + "\n").toStdString();
        }
    }
    if(command.user->GetUseLikedAuthorsOnly())
        field.value += "\nLiked authors filter is active.";
    if(command.user->GetSortFreshFirst())
        field.value += "\nFresh recommendations sorting is active.";
    embed.fields.push_back(field);
}

QSharedPointer<SendMessageCommand> DisplayPageAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    //QLOG_TRACE() << "Creating page results";

    environment->ficSource->ClearUserData();
    auto page = command.ids.at(0);
    command.user->SetPage(page);
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->UpdateCurrentPage(command.user->UserID(), page);

    QVector<core::Fanfic> fics;
    QLOG_TRACE() << "Fetching fics";
    environment->ficSource->ClearUserData();
    static constexpr int listSize = 9;
    FetchFicsForDisplayPageCommand(environment->ficSource, command.user, listSize, &fics);
    int pageCount = FetchPageCountForFilterCommand(environment->ficSource, command.user, listSize);
    auto userFics = command.user->FicList();
    for(auto& fic : fics)
        fic.score = userFics->ficToScore.value(fic.identity.id);
    //QLOG_TRACE() << "Fetched fics";
    SleepyDiscord::Embed embed;
    //QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";
    //QString authorUrlProto = "[%1](https://www.fanfiction.net/u/%2)";
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    environment->fandoms->FetchFandomsForFics(&fics);
    auto editPreviousPageIfPossible = command.variantHash.value(QStringLiteral("refresh_previous")).toBool();

    if(command.targetMessage.string().length() != 0){
        action->text = QString::fromStdString(CreateMention(command.user->UserID().toStdString()) + ", here are the results:");
    }
    else{
        auto previousPage = command.user->GetLastPageMessage();
        if(editPreviousPageIfPossible && previousPage.message.string().length() > 0 && previousPage.channel == command.originalMessageToken.channelID)
        {
            action->text = QString::fromStdString(CreateMention(command.originalMessageToken.authorID.string()) + ", here are the results:");
            action->diagnosticText = QString::fromStdString(CreateMention(command.originalMessageToken.authorID.string()) + ", your previous results have been updated with new data." );
            action->targetMessage = previousPage.message;
        }
        else
            action->text = QString::fromStdString(CreateMention(command.originalMessageToken.authorID.string()) + ", here are the results:");
        //        action->text = QString::fromStdString(CreateMention(command.originalMessage.author.ID.string()) + ", here are the results:");
    }

    embed.description = QString(QStringLiteral("Generated recs for user [%1](https://www.fanfiction.net/u/%1), page: %2 of %3")).arg(command.user->FfnID()).arg(command.user->CurrentRecommendationsPage()).arg(QString::number(pageCount)).toStdString();
    auto& tips = SendMessageCommand::tips;
    int tipNumber =  rand() % tips.size();
    static constexpr uint8_t promoFrequency = 7;
    bool showAppOrPatreon = rand() % promoFrequency == 0;
    SleepyDiscord::EmbedFooter footer;
    if(showAppOrPatreon){
        QStringList appPromo =  {QStringLiteral("Socrates has a PC desktop app version called Flipper that has more filters and is more convenient to use. You can get it at https://github.com/Zeks/flipper/releases/latest"),
                                 QStringLiteral("If you would like to support the bot, you can do it on https://www.patreon.com/Zekses")};
        int shownId = rand() %2 == 0;
        footer.text = appPromo.at(shownId).toStdString();
        if(shownId == 1)
            footer.iconUrl = "https://c5.patreon.com/external/logo/downloads_logomark_color_on_white@2x.png";
        else
            footer.iconUrl = "https://github.githubassets.com/images/modules/logos_page/GitHub-Mark.png";
    }
    else{
        if(tips.size() > 0)
        {
            auto temp = tips.at(tipNumber);
            if(temp.contains(QStringLiteral("%1")))
                temp =temp.arg(QSV(command.server->GetCommandPrefix()));
            footer.text = temp .toStdString();
        }
    }
    embed.footer = footer;

    FillActiveFilterPartInEmbed(embed, environment, command);

    QHash<int, int> positionToId;
    int i = 0;
    for(auto fic: std::as_const(fics))
    {
        positionToId[i+1] = fic.identity.id;
        i++;
        FillListEmbedForFicAsFields(embed, fic, i);
    }

    command.user->SetPositionsToIdsForCurrentPage(positionToId);
    command.user->SetLastPageType(ct_display_page);
    action->embed = embed;
    action->reactionsToAdd.push_back(QStringLiteral("%f0%9f%91%88"));
    action->reactionsToAdd.push_back(QStringLiteral("%f0%9f%91%89"));
    if(command.targetMessage.string().length() > 0)
        action->targetMessage = command.targetMessage;
    environment->ficSource->ClearUserData();
    QLOG_INFO() << QStringLiteral("Created page results");
    return action;
}


QSharedPointer<SendMessageCommand> DisplayRngAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto quality = command.variantHash.value(QStringLiteral("quality")).toString().trimmed();
    if(quality.length() == 0)
        quality = command.user->GetLastUsedRoll().isEmpty() ? QStringLiteral("all ") : command.user->GetLastUsedRoll();
    QVector<core::Fanfic> fics;
    command.user->SetLastUsedRoll(quality);

    //QSet<int> filteringFicSet;
    int scoreCutoff = 1;
    if(quality == QStringLiteral("best"))    {
        scoreCutoff= command.user->GetPerfectRngScoreCutoff();
    }
    if(quality == QStringLiteral("good")){
        scoreCutoff= command.user->GetGoodRngScoreCutoff();
    }

    QLOG_TRACE() << QStringLiteral("Fetching fics for rng");
    FetchFicsForDisplayRngCommand(3, environment->ficSource, command.user, &fics, scoreCutoff);
    auto userFics = command.user->FicList();
    for(auto& fic : fics)
        fic.score = userFics->ficToScore.value(fic.identity.id);

    QLOG_TRACE() << QStringLiteral("Fetched fics for rng");

    // fetching fandoms for selected fics
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    environment->fandoms->FetchFandomsForFics(&fics);

    SleepyDiscord::Embed embed;
    //QString urlProto = "[%1](https://www.fanfiction.net/s/%2)";
    auto editPreviousPageIfPossible = command.variantHash.value(QStringLiteral("refresh_previous")).toBool();
    if(command.targetMessage.string().length() != 0)
        action->text = QString::fromStdString(CreateMention(command.user->UserID().toStdString()) + ", here are the results:");
    else {
        auto previousPage = command.user->GetLastPageMessage();
        if(editPreviousPageIfPossible && previousPage.message.string().length() > 0 && previousPage.channel == command.originalMessageToken.channelID)
        {
            action->text = QString::fromStdString(CreateMention(command.originalMessageToken.authorID.string()) + ", here are the results:");
            action->diagnosticText = QString::fromStdString(CreateMention(command.originalMessageToken.authorID.string()) + ", your previous results have been updated with new data." );
            action->targetMessage = previousPage.message;
        }
        else
            action->text = QString::fromStdString(CreateMention(command.originalMessageToken.authorID.string()) + ", here are the results:");
    }


    QHash<int, int> positionToId;
    int i = 0;
    for(auto fic: std::as_const(fics))
    {
        positionToId[i+1] = fic.identity.id;
        i++;
        FillDetailedEmbedForFic(embed, fic, i,false);
    }

    command.user->SetPositionsToIdsForCurrentPage(positionToId);
    command.user->SetLastPageType(ct_display_rng);
    FillActiveFilterPartInEmbed(embed, environment, command);

    action->embed = embed;
    action->reactionsToAdd.push_back(QStringLiteral("%f0%9f%94%81"));
    if(command.targetMessage.string().length() > 0)
        action->targetMessage = command.targetMessage;
    QLOG_INFO() << QStringLiteral("Created page results");
    return action;


}


QSharedPointer<SendMessageCommand> SetFandomAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto fandom = command.variantHash.value(QStringLiteral("fandom")).toString().trimmed();
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    auto fandomId = environment->fandoms->GetIDForName(fandom);
    auto currentFilter = command.user->GetCurrentFandomFilter();
    An<interfaces::Users> usersDbInterface;
    if(command.variantHash.contains(QStringLiteral("reset")))
    {
        usersDbInterface->ResetFandomFilter(command.user->UserID());
        command.user->ResetFandomFilter();
        action->emptyAction = true;
        return action;
    }
    if(fandomId == -1)
    {
        action->text = QStringLiteral("`") + fandom  + QStringLiteral("` is not a valid fandom");
        action->stopChain = true;
        return action;
    }
    if(currentFilter.fandoms.contains(fandomId))
    {
        currentFilter.RemoveFandom(fandomId);
        action->text = QStringLiteral("Removing filtered fandom: ") + fandom;
        usersDbInterface->UnfilterFandom(command.user->UserID(), fandomId);
    }
    else if(currentFilter.fandoms.size() == 2)
    {
        auto oldFandomId = std::as_const(currentFilter.tokens).last().id;
        action->text = QStringLiteral("Replacing crossover fandom: ") + environment->fandoms->GetNameForID(currentFilter.tokens.last().id) +    QStringLiteral(" with: ") + fandom;
        usersDbInterface->UnfilterFandom(command.user->UserID(), oldFandomId);
        usersDbInterface->FilterFandom(command.user->UserID(), fandomId, command.variantHash.value(QStringLiteral("allow_crossovers")).toBool());
        currentFilter.RemoveFandom(oldFandomId);
        currentFilter.AddFandom(fandomId, command.variantHash.value(QStringLiteral("allow_crossovers")).toBool());
        return action;
    }
    else {
        usersDbInterface->FilterFandom(command.user->UserID(), fandomId, command.variantHash.value(QStringLiteral("allow_crossovers")).toBool());
        currentFilter.AddFandom(fandomId, command.variantHash.value(QStringLiteral("allow_crossovers")).toBool());
    }
    command.user->SetRngBustScheduled(true);
    command.user->SetFandomFilter(currentFilter);
    action->emptyAction = true;
    return action;
}

QSharedPointer<SendMessageCommand> IgnoreFandomAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto fandom = command.variantHash.value(QStringLiteral("fandom")).toString().trimmed();
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    auto fandomId = environment->fandoms->GetIDForName(fandom);
    auto properNameForFandom = environment->fandoms->GetNameForID(fandomId);

    auto withCrossovers = command.variantHash.value(QStringLiteral("with_crossovers")).toBool();
    An<interfaces::Users> usersDbInterface;
    if(command.variantHash.contains(QStringLiteral("reset")))
    {
        usersDbInterface->ResetFandomIgnores(command.user->UserID());
        command.user->ResetFandomIgnores();
        action->emptyAction = true;
        return action;
    }
    if(fandomId == -1)
    {
        action->text = QStringLiteral("`") + fandom  + QStringLiteral("` is not a valid fandom");
        action->stopChain = true;
        return action;
    }

    auto currentIgnores = command.user->GetCurrentIgnoredFandoms();
    auto ignoreFandom = [&](){
        usersDbInterface->IgnoreFandom(command.user->UserID(), fandomId, withCrossovers);
        currentIgnores.AddFandom(fandomId, withCrossovers);
        action->text = QStringLiteral("Adding fandom: ") + properNameForFandom + QStringLiteral(" to ignores");
        if(withCrossovers)
            action->text += QStringLiteral(".Will also exclude crossovers from now on.");
        else
            action->text += QStringLiteral(".");
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
            action->text = QStringLiteral("Removing fandom: ") + properNameForFandom + QStringLiteral(" from ignores");
        }
    }
    else{
        ignoreFandom();

    }
    command.user->SetRngBustScheduled(true);
    command.user->SetIgnoredFandoms(currentIgnores);
    //action->emptyAction = true;
    return action;
}

QSharedPointer<SendMessageCommand> IgnoreFicAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    auto ficIds = command.ids;
    if(command.variantHash.contains(QStringLiteral("everything")))
        ficIds = {1,2,3,4,5,6,7,8,9,10};

    auto ignoredFics = command.user->GetIgnoredFics();
    An<interfaces::Users> usersDbInterface;
    QStringList ignoredIds;

    for(auto positionalId : std::as_const(ficIds)){
        //need to get ffn id from positional id
        auto ficId = command.user->GetFicIDFromPositionId(positionalId);
        if(ficId != -1){
            if(!ignoredFics.contains(ficId))
            {
                ignoredFics.insert(ficId);
                usersDbInterface->TagFanfic(command.user->UserID(), QStringLiteral("ignored"),  ficId);
                ignoredIds.push_back(QString::number(positionalId));
            }
            else{
                ignoredFics.remove(ficId);
                usersDbInterface->UnTagFanfic(command.user->UserID(), QStringLiteral("ignored"),  ficId);
            }
        }
    }
    command.user->SetIgnoredFics(ignoredFics);
    action->text = QStringLiteral("Ignored fics: ") + ignoredIds.join(QStringLiteral(" "));
    return action;
}

QSharedPointer<SendMessageCommand> TimeoutActiveAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    auto reason = command.variantHash.value(QStringLiteral("reason")).toString();
    auto seconds= command.ids.at(0);
    action->text = reason .arg(QString::number(seconds));
    return action;
}

QSharedPointer<SendMessageCommand> NoUserInformationAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    action->text = QString(QStringLiteral("You need to call %1recs FFN_ID first")).arg(QSV(command.server->GetCommandPrefix()));
    return action;
}

QSharedPointer<SendMessageCommand> ChangePrefixAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    if(command.variantHash.contains(QStringLiteral("prefix"))){
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
        command.server->SetCommandPrefix(command.variantHash.value(QStringLiteral("prefix")).toString().trimmed().toStdString());
        database::discord_queries::WriteServerPrefix(dbToken->db, command.server->GetServerId(), QSV(command.server->GetCommandPrefix()).trimmed());
        action->text = QStringLiteral("Prefix has been changed");
    }
    else
        action->text = QStringLiteral("Prefix wasn't changed because of an error");
    return action;
}

QSharedPointer<SendMessageCommand> SetChannelAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    if(command.variantHash.contains(QStringLiteral("channel"))){
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
        command.server->SetDedicatedChannelId(command.variantHash.value(QStringLiteral("channel")).toString().trimmed().toStdString());
        database::discord_queries::WriteServerDedicatedChannel(dbToken->db, command.server->GetServerId(), command.server->GetDedicatedChannelId());
        action->text = QStringLiteral("Acknowledged, the bot will only respond in this channel.");
    }
    else
        action->text = QStringLiteral("Prefix wasn't changed because of an error");
    return action;
}

QSharedPointer<SendMessageCommand> InsufficientPermissionsAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&&)
{
    action->text = QStringLiteral("You don't have required permissions on the server to run this command.");
    return action;
}
QSharedPointer<SendMessageCommand> NullAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    if(command.variantHash.contains(QStringLiteral("reason")))
        action->text = command.variantHash[QStringLiteral("reason")].toString();
    return action;
}

QSharedPointer<SendMessageCommand> SetForcedListParamsAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->WriteForcedListParams(command.user->UserID(), command.variantHash.value(QStringLiteral("min")).toUInt(), command.variantHash.value(QStringLiteral("ratio")).toUInt());
    command.user->SetForcedMinMatch(command.variantHash.value(QStringLiteral("min")).toUInt());
    command.user->SetForcedRatio(command.variantHash.value(QStringLiteral("ratio")).toUInt());
    return action;
}

QSharedPointer<SendMessageCommand> SetForceLikedAuthorsAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    An<interfaces::Users> usersDbInterface;
    if(command.user->GetUseLikedAuthorsOnly())
        usersDbInterface->WriteForceLikedAuthors(command.user->UserID(), false);
    else
        usersDbInterface->WriteForceLikedAuthors(command.user->UserID(), true);
    command.user->SetUseLikedAuthorsOnly(!command.user->GetUseLikedAuthorsOnly());

    return action;
}


QSharedPointer<SendMessageCommand> ShowFreshRecommendationsAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    An<interfaces::Users> usersDbInterface;
    bool strict = command.variantHash.contains(QStringLiteral("strict"));
    if(!command.user->GetSortFreshFirst() ||
            (strict && command.user->GetSortFreshFirst() && !command.user->GetStrictFreshSort())){
        usersDbInterface->WriteFreshSortingParams(command.user->UserID(), true, strict);
        action->text = QStringLiteral("Fresh sorting mode turned on, to disable use the same command again.");
        if(command.user->GetSortFreshFirst())
            action->text = QStringLiteral("Enabling strict mode for fresh sort.");
        command.user->SetSortFreshFirst(true);
        command.user->SetStrictFreshSort(strict);
    }
    else{
        usersDbInterface->WriteFreshSortingParams(command.user->UserID(), false, false);
        command.user->SetSortFreshFirst(false);
        command.user->SetStrictFreshSort(false);
        action->text = QStringLiteral("Disabling fresh sort.");
    }
    return action;
}



QSharedPointer<SendMessageCommand> ShowCompleteAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    An<interfaces::Users> usersDbInterface;
    auto user = command.user;
    if(!user->GetShowCompleteOnly()){
        usersDbInterface->SetCompleteFilter(command.user->UserID(), true);
        user->SetShowCompleteOnly(true);
    }
    else{
        usersDbInterface->SetCompleteFilter(command.user->UserID(), false);
        user->SetShowCompleteOnly(false);
    }
    user->SetRngBustScheduled(true);
    return action;
}


QSharedPointer<SendMessageCommand> HideDeadAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    auto user = command.user;
    An<interfaces::Users> usersDbInterface;
    if(command.variantHash.contains(QStringLiteral("days"))){
        int days = command.variantHash.value(QStringLiteral("days")).toUInt();
        user->SetDeadFicDaysRange(days);
        usersDbInterface->SetDeadFicDaysRange(command.user->UserID(), days);
        usersDbInterface->SetHideDeadFilter(command.user->UserID(), true);
        user->SetHideDead(true);
    }
    else{
        if(!user->GetHideDead()){
            usersDbInterface->SetHideDeadFilter(command.user->UserID(), true);
            user->SetHideDead(true);
        }
        else{
            usersDbInterface->SetHideDeadFilter(command.user->UserID(), false);
            user->SetHideDead(false);
        }
    }
    user->SetRngBustScheduled(true);
    return action;
}


QSharedPointer<SendMessageCommand> PurgeAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    An<interfaces::Users> usersDbInterface;
    auto user = command.user;
    An<Users> users;
    users->RemoveUserData(user);
    usersDbInterface->CompletelyRemoveUser(user->UserID());
    action->text = QStringLiteral("Acknowledged: removing all your data from the database");
    return action;
}

QSharedPointer<SendMessageCommand> ResetFiltersAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    environment->fandoms->db = dbToken->db;
    An<interfaces::Users> usersDbInterface;
    auto user = command.user;
    user->SetHideDead(false);
    user->SetShowCompleteOnly(false);
    user->SetSortFreshFirst(false);
    user->SetStrictFreshSort(false);
    user->SetUseLikedAuthorsOnly(false);
    user->SetWordcountFilter({0,0});
    {
        usersDbInterface->SetHideDeadFilter(command.user->UserID(), false);
        usersDbInterface->SetCompleteFilter(command.user->UserID(), false);
        usersDbInterface->WriteFreshSortingParams(command.user->UserID(), false, false);
        usersDbInterface->WriteForceLikedAuthors(command.user->UserID(), false);
        usersDbInterface->SetWordcountFilter(command.user->UserID(), {0,0});
        auto currentFilter = command.user->GetCurrentFandomFilter();
        for(auto fandomId : std::as_const(currentFilter.fandoms))
            usersDbInterface->UnfilterFandom(command.user->UserID(), fandomId);
    }
    user->SetFandomFilter({});
    user->SetRngBustScheduled(true);
    action->text = QStringLiteral("Done, filter has been reset.");
    return action;
}


QSharedPointer<SendMessageCommand> CreateSimilarFicListAction::ExecuteImpl(QSharedPointer<TaskEnvironment> environment, Command&& command)
{
    command.user->initNewEasyQuery();
    command.user->SetRngBustScheduled(true);
    auto ficId = command.ids.at(0);
    QSharedPointer<core::RecommendationList> listParams;
    //QString error;

    auto recList = CreateSimilarFicParams();
    recList->ignoreBreakdowns= true;

    QVector<core::Identity> pack;
    pack.resize(1);
    pack[0].web.ffn = ficId;
    environment->ficSource->ClearUserData();
    environment->ficSource->GetInternalIDsForFics(&pack);

    bool hasValidId = false;
    for(const auto& source: std::as_const(pack))
    {
        if(source.id > 0 )
            hasValidId = true;
        recList->ficData->sourceFics+=source.id;
        recList->ficData->fics+=source.web.ffn;
    }
    if(!hasValidId)
    {
        action->text = QStringLiteral("Couldn't create the list. Server doesn't have any data for fic ID you supplied, either it's too new or too unpopular.");
        action->stopChain = true;
        return action;
    }

    environment->ficSource->GetRecommendationListFromServer(recList);

    // instantiating working set for user
    An<Users> users;
    command.user->SetFicList(recList->ficData);
    //QMap<int, int> scoreStatus; // maps maptch count to fic count with this match
    //QMap<int, QSet<int>> matchFicToScore; // maps maptch count to fic count with this match

    if(recList->ficData->matchCounts.isEmpty())
    {
        action->text = QStringLiteral("Couldn't create the list. Recommendations server is not available?");
        action->stopChain = true;
        return action;
    }


    action->text = QStringLiteral("Created similarity list FFN ID: ") + QString::number(command.ids.at(0));
    environment->ficSource->ClearUserData();
    return action;
}

QSharedPointer<SendMessageCommand> SetWordcountLimitAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&& command)
{
    //auto ficIds = command.ids;
    static constexpr int thousandWords = 1000;
    command.user->SetWordcountFilter({command.ids.at(0)*1000,command.ids.at(1)*thousandWords});
    An<interfaces::Users> usersDbInterface;
    usersDbInterface->SetWordcountFilter(command.user->UserID(),command.user->GetWordcountFilter());
    return action;
}



QSharedPointer<ActionBase> GetAction(ECommandType type)
{
    switch(type){
    case ECommandType::ct_ignore_fics:
        return QSharedPointer<ActionBase>(new IgnoreFicAction());
    case ECommandType::ct_ignore_fandoms:
        return QSharedPointer<ActionBase>(new IgnoreFandomAction());
    case ECommandType::ct_set_fandoms:
        return QSharedPointer<ActionBase>(new SetFandomAction());
    case ECommandType::ct_display_help:
        return QSharedPointer<ActionBase>(new GeneralHelpAction());
    case ECommandType::ct_display_page:
        return QSharedPointer<ActionBase>(new DisplayPageAction());
    case ECommandType::ct_timeout_active:
        return QSharedPointer<ActionBase>(new TimeoutActiveAction());
    case ECommandType::ct_fill_recommendations:
        return QSharedPointer<ActionBase>(new DesktopRecsCreationAction());
    case ECommandType::ct_no_user_ffn:
        return QSharedPointer<ActionBase>(new NoUserInformationAction());
    case ECommandType::ct_display_rng:
        return QSharedPointer<ActionBase>(new DisplayRngAction());
    case ECommandType::ct_change_server_prefix:
        return QSharedPointer<ActionBase>(new ChangePrefixAction());
    case ECommandType::ct_insufficient_permissions:
        return QSharedPointer<ActionBase>(new InsufficientPermissionsAction());
    case ECommandType::ct_force_list_params:
        return QSharedPointer<ActionBase>(new SetForcedListParamsAction());
    case ECommandType::ct_filter_liked_authors:
        return QSharedPointer<ActionBase>(new SetForceLikedAuthorsAction());
    case ECommandType::ct_show_favs:
        return QSharedPointer<ActionBase>(new ShowFullFavouritesAction());
    case ECommandType::ct_filter_fresh:
        return QSharedPointer<ActionBase>(new ShowFreshRecommendationsAction());
    case ECommandType::ct_filter_complete:
        return QSharedPointer<ActionBase>(new ShowCompleteAction());
    case ECommandType::ct_filter_out_dead:
        return QSharedPointer<ActionBase>(new HideDeadAction());
    case ECommandType::ct_purge:
        return QSharedPointer<ActionBase>(new PurgeAction());
    case ECommandType::ct_reset_filters:
        return QSharedPointer<ActionBase>(new ResetFiltersAction());
    case ECommandType::ct_create_similar_fics_list:
        return QSharedPointer<ActionBase>(new CreateSimilarFicListAction());
    case ECommandType::ct_create_recs_from_mobile_page:
        return QSharedPointer<ActionBase>(new MobileRecsCreationAction());
    case ECommandType::ct_set_wordcount_limit:
        return QSharedPointer<ActionBase>(new SetWordcountLimitAction());
    case ECommandType::ct_set_permitted_channel:
        return QSharedPointer<ActionBase>(new SetChannelAction());

    default:
        return QSharedPointer<ActionBase>(new NullAction());
    }
}

QSharedPointer<SendMessageCommand> ShowFullFavouritesAction::ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&&)
{
    return {};
}









}









