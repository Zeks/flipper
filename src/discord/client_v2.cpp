#include "discord/client_v2.h"
#include "discord/command_controller.h"
#include "logger/QsLog.h"
namespace discord {

Client::Client(const std::string token, const char numOfThreads, QObject *obj):QObject(obj),
    SleepyDiscord::DiscordClient(token, numOfThreads)
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    startTimer(60000);

}

Client::Client(QObject *obj):QObject(obj), SleepyDiscord::DiscordClient()
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
}

void Client::InitHelpForCommands(){
    QString helpString = "Basic commands:\n`!recs FFN_ID` to create recommendations";
    CommandState<NextPageCommand>::help = "`!next` to navigate to the next page of the recommendation results";
    CommandState<PreviousPageCommand>::help = "`!prev` to navigate to the previous page of the recommendation results";
    CommandState<PageChangeCommand>::help = "`!page X` to navigate to a differnt page in recommendation results";
    CommandState<SetFandomCommand>::help = "\nFandom filter commands:\n`!fandom X` for single fandom searches"
                                           "\n`!fandom #pure X` if you want to exclude crossovers "
                                           "\n`!fandom` a second time with a diffent fandom if you want to search for exact crossover"
                                           "\n`!fandom #reset` to reset fandom filter";
    CommandState<IgnoreFandomCommand>::help = "\nFandom ignore commands:\n`!xfandom X` to permanently ignore fics just from this fandom or remove an ignore"
                                              "\n`!xfandom #full X` to also ignore crossovers from this fandom,"
                                              "\n`!xfandom #reset` to reset fandom ignore list";
    //CommandState<IgnoreFandomWithCrossesCommand>::help = "xcrossfandom to permanently ignore a fandom eve when it appears in crossovers, repeat to unignore";
    CommandState<IgnoreFicCommand>::help = "\nFanfic commands:\n`!xfic X` will ignore a fic (you need input position in the last output), X Y Z to ignore multiple";
    CommandState<DisplayHelpCommand>::help = "`!help` display this text";
}

void Client::InitDefaultCommandSet()
{
    RegisterCommand<RecsCreationCommand>();
    RegisterCommand<PageChangeCommand>();
    RegisterCommand<NextPageCommand>();
    RegisterCommand<PreviousPageCommand>();
    //RegisterCommand<SetFandomCommand>();
    RegisterCommand<IgnoreFandomCommand>();
    //RegisterCommand<IgnoreFicCommand>();
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
    if(message.content.at(0) != '!')
        return;
    auto commands = parser.Execute(message);
    if(commands.Size() == 0)
        return;
    executor->Push(commands);
}

void Client::Log(const SleepyDiscord::Message message)
{
    QLOG_INFO() << message.serverID.number() << " " << message.channelID.number() << " " << QString::fromStdString(message.author.username) << QString::number(message.author.ID.number()) << " " << QString::fromStdString(message.content);
}

void Client::timerEvent(QTimerEvent *)
{
    An<discord::Users> users;
    users->ClearInactiveUsers();
}

}

