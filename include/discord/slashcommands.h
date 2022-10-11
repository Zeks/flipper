#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#include "sleepy_discord/user.h"
#include "sleepy_discord/slash_commands.h"
#pragma GCC diagnostic pop


namespace discord {
class Client;
}
namespace discord {
namespace slash_commands{
    enum class ECommandTypes{
      ct_invalid_command = 0,
      ct_recs_creation   = 1

    };
    struct CommandDispatcher{
        public:
        CommandDispatcher() = delete;
        CommandDispatcher(discord::Client* client,
                          SleepyDiscord::Snowflake<SleepyDiscord::User>);
        void InitAllSlashCommands();
        void ProcessAnySlashCommand(SleepyDiscord::Interaction &&) const;
        void ProcessRecsSlashCommand(SleepyDiscord::Interaction &&) const;
    private:
        ECommandTypes StringToCommandType(std::string) const;
        void InitRecsSlashCommand() const;
        SleepyDiscord::Message CreateMessageTemplateFromInteraction(const SleepyDiscord::Interaction&) const;
        discord::Client* client = nullptr;
        SleepyDiscord::Snowflake<SleepyDiscord::User> botUser;
    };


}
}
