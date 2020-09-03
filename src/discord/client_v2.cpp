#include "discord/client_v2.h"
namespace discord {

Client::Client(const std::string token, const char numOfThreads, QObject *obj):QObject(obj),
    SleepyDiscord::DiscordClient(token, numOfThreads)
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");

}

Client::Client(QObject *obj):QObject(obj), SleepyDiscord::DiscordClient()
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
}

void InitHelpForCommands(){
    QString helpString = "!recs FFN_ID to create recommendations";
    CommandState<NextPageCommand>::help = "!next to navigate to the next page of the recommendation results";
    CommandState<PreviousPageCommand>::help = "!prev to navigate to the previous page of the recommendation results";
    CommandState<PageChangeCommand>::help = "!page X to navigate to a differnt page in recommendation results";
    CommandState<SetFandomCommand>::help = "!fandom for fandom searches, do it a second time to add crossover, repeat the fandom to remove it, fandom must be entered exactly the same as the bot shows it";
    CommandState<IgnoreFandomCommand>::help = "!xpurefandom to permanently ignore fic just from this fandom, repeat to unignore, fandom must be entered exactly the same as the bot shows it";
    CommandState<IgnoreFandomWithCrossesCommand>::help = "xcrossfandom to permanently ignore a fandom eve when it appears in crossovers, repeat to unignore";
    CommandState<IgnoreFicCommand>::help = "!xfic X will ignore a fic (you need input position in the last output), X Y Z to ignore multiple";
    CommandState<DisplayHelpCommand>::help = "!help display this text";
}

void Client::InitDefaultCommandSet()
{
    RegisterCommand<RecsCreationCommand>();
    RegisterCommand<PageChangeCommand>();
    RegisterCommand<NextPageCommand>();
    RegisterCommand<PreviousPageCommand>();
    RegisterCommand<SetFandomCommand>();
    RegisterCommand<IgnoreFandomCommand>();
    RegisterCommand<IgnoreFicCommand>();
    RegisterCommand<SetIdentityCommand>();
    RegisterCommand<DisplayHelpCommand>();
}
void Client::onMessage(SleepyDiscord::Message message) {
    Log(message);
    auto commands = parser.Execute(message);
    if(commands.Size() == 0)
        return;
}

void Client::Log(const SleepyDiscord::Message)
{

}

}

