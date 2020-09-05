#include "discord/client_v2.h"
#include "discord/command_controller.h"
#include "logger/QsLog.h"
#include <QRegularExpression>
namespace discord {

Client::Client(const std::string token, const char numOfThreads, QObject *obj):QObject(obj),
    SleepyDiscord::DiscordClient(token, numOfThreads)
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    startTimer(60000);
    parser.client = this;

}

Client::Client(QObject *obj):QObject(obj), SleepyDiscord::DiscordClient()
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    parser.client = this;
}

void Client::InitHelpForCommands(){
    QString helpString = "Basic commands:\n`!recs FFN_ID` to create recommendations";
    CommandState<NextPageCommand>::help = "`!next` to navigate to the next page of the recommendation results";
    CommandState<PreviousPageCommand>::help = "`!prev` to navigate to the previous page of the recommendation results";
    CommandState<PageChangeCommand>::help = "`!page X` to navigate to a differnt page in recommendation results";
    CommandState<SetFandomCommand>::help = "\nFandom filter commands:\n`!fandom X` for single fandom searches"
                                           "\n`!fandom >pure X` if you want to exclude crossovers "
                                           "\n`!fandom` a second time with a diffent fandom if you want to search for exact crossover"
                                           "\n`!fandom >reset` to reset fandom filter";
    CommandState<IgnoreFandomCommand>::help = "\nFandom ignore commands:\n`!xfandom X` to permanently ignore fics just from this fandom or remove an ignore"
                                              "\n`!xfandom >full X` to also ignore crossovers from this fandom,"
                                              "\n`!xfandom >reset` to reset fandom ignore list";
    //CommandState<IgnoreFandomWithCrossesCommand>::help = "xcrossfandom to permanently ignore a fandom eve when it appears in crossovers, repeat to unignore";
    CommandState<IgnoreFicCommand>::help = "\nFanfic commands:\n`!xfic X` will ignore a fic (you need input position in the last output), X Y Z to ignore multiple"
                                           "\n`!xfic >all` will ignore the whole page";
                                           //"\n`!xfic >reset` resets the fic ignores";
    CommandState<DisplayHelpCommand>::help = "`!help` display this text";
}
template<typename T> QString GetRegexForCommandIfActive(){
    QString result;
    if(CommandState<T>::active)
        result = CommandState<T>::regexCommandIdentifier;
    return result;
}
void Client::InitIdentifiersForCommands(){

    CommandState<RecsCreationCommand>::regexCommandIdentifier= "recs";
    CommandState<NextPageCommand>::regexCommandIdentifier = "next";
    CommandState<PreviousPageCommand>::regexCommandIdentifier = "prev";
    CommandState<PageChangeCommand>::regexCommandIdentifier = "page";
    CommandState<SetFandomCommand>::regexCommandIdentifier = "fandom";
    CommandState<IgnoreFandomCommand>::regexCommandIdentifier = "xfandom";
    CommandState<IgnoreFicCommand>::regexCommandIdentifier = "xfic";
    CommandState<DisplayHelpCommand>::regexCommandIdentifier = "help";

    QString pattern = "^!(%1)";
    QStringList list;
    list.push_back(GetRegexForCommandIfActive<RecsCreationCommand>());
    list.push_back(GetRegexForCommandIfActive<NextPageCommand>());
    list.push_back(GetRegexForCommandIfActive<PreviousPageCommand>());
    list.push_back(GetRegexForCommandIfActive<PageChangeCommand>());
    list.push_back(GetRegexForCommandIfActive<SetFandomCommand>());
    list.push_back(GetRegexForCommandIfActive<IgnoreFandomCommand>());
    list.push_back(GetRegexForCommandIfActive<IgnoreFicCommand>());
    list.push_back(GetRegexForCommandIfActive<DisplayHelpCommand>());
    list.removeAll("");
    pattern = pattern.arg(list.join("|"));
    QLOG_INFO() << "Created command match pattern: " << pattern;
    rxCommandIdentifier = std::regex(pattern.toStdString());
}

void Client::InitDefaultCommandSet()
{
    RegisterCommand<RecsCreationCommand>();
    RegisterCommand<PageChangeCommand>();
    RegisterCommand<NextPageCommand>();
    RegisterCommand<PreviousPageCommand>();
    //RegisterCommand<SetFandomCommand>();
    RegisterCommand<IgnoreFandomCommand>();
    RegisterCommand<IgnoreFicCommand>();
    //RegisterCommand<SetIdentityCommand>();
    RegisterCommand<DisplayHelpCommand>();
}

void Client::InitCommandExecutor()
{
    executor.reset(new CommandController());
    executor->client = this;
}
void Client::onMessage(SleepyDiscord::Message message) {
    if(message.author.bot)
        return;
    Log(message);
    if(!message.content.size() || (message.content.size() && !std::regex_search(message.content, rxCommandIdentifier)))
        return;
    auto commands = parser.Execute(message);
    if(commands.Size() == 0)
        return;
    executor->Push(commands);
}

void Client::Log(const SleepyDiscord::Message message)
{
    QLOG_INFO() << " " << message.channelID.number() << " " << QString::fromStdString(message.author.username) << QString::number(message.author.ID.number()) << " " << QString::fromStdString(message.content);
}

void Client::timerEvent(QTimerEvent *)
{
    An<discord::Users> users;
    users->ClearInactiveUsers();
}

}


