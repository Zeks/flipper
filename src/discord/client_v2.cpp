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

