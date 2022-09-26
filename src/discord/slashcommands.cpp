#include "discord/slashcommands.h"
#include "discord/client_v2.h"







namespace discord {
namespace slash_commands{

CommandDispatcher::CommandDispatcher(Client *client, SleepyDiscord::Snowflake<SleepyDiscord::User> user)
{
    this->client = client;
    this->botUser = user;
}

void CommandDispatcher::InitAllSlashCommands()
{
    InitRecsSlashCommand();
}

void CommandDispatcher::ProcessAnySlashCommand(SleepyDiscord::Interaction && interaction) const
{
    switch(StringToCommandType(interaction.data.name)){
        case slash_commands::ECommandTypes::ct_recs_creation:
                ProcessRecsSlashCommand(std::move(interaction));
        break;
    default:
        break;
    }
}

template<typename T>
struct OptionalWithTemp{
    std::optional<T> optionalValue;
    T rawvalue;
    void Process(bool hasValue = false){
        if(hasValue)
            optionalValue = rawvalue;
    }
    bool has_value(){return optionalValue.has_value();}
    T value_or(T&& defaultValue){return optionalValue.value_or(defaultValue);}
};

void CommandDispatcher::ProcessRecsSlashCommand(SleepyDiscord::Interaction && interaction) const
{
    SleepyDiscord::Interaction::Response<> response;
    response.type = SleepyDiscord::InteractionCallbackType::ChannelMessageWithSource;

    OptionalWithTemp<std::string> id;
    OptionalWithTemp<std::string> refresh;
    for (auto& option : interaction.data.options) {
        if (option.name == "id") {
            id.Process(option.get(id.rawvalue));
            }
        if (option.name == "refresh") {
            id.Process(option.get(refresh.rawvalue));
            }
        }
    if(id.has_value()){
        auto message  = CreateMessageTemplateFromInteraction(interaction);
        message.content = "!recs ";
        if(refresh.value_or("") == "Y")
            message.content += ">refresh ";
        message.content += *id.optionalValue;;
        response.data.content = "executing";
        client->createInteractionResponse(interaction.ID, interaction.token, response);
        client->onMessage(message);
    }
    else{
        response.data.content = "incorrect paramateres to the command";
        client->createInteractionResponse(interaction.ID, interaction.token, response);
    }
}

ECommandTypes CommandDispatcher::StringToCommandType(std::string value) const
{
    if(value == "recs")
        return ECommandTypes::ct_recs_creation;
    return ECommandTypes::ct_invalid_command;
}

void discord::slash_commands::CommandDispatcher::InitRecsSlashCommand() const
{
    const std::string name = "recs";
    const std::string description = "Creates recommendation list";
    std::vector<SleepyDiscord::AppCommand::Option> options;

    SleepyDiscord::AppCommand::Option id;
    id.name = "id";
    id.isRequired = true;
    id.description = "Id of your ffn profile or url";
    id.type = SleepyDiscord::AppCommand::Option::TypeHelper<std::string>::getType();
    options.push_back(std::move(id));

    SleepyDiscord::AppCommand::Option refresh;
    refresh.name = "refresh";
    refresh.isRequired = false;
    refresh.description = "Do you need to refresh recommendations? Y/N (don't use this often)";
    refresh.type = SleepyDiscord::AppCommand::Option::TypeHelper<std::string>::getType();
    options.push_back(std::move(refresh));

    try{
        client->createGlobalAppCommand(botUser, name, description, std::move(options));
    }
    catch (const SleepyDiscord::ErrorCode& error){
        QLOG_INFO() << "Discord error:" << error;
    }
}

SleepyDiscord::Message CommandDispatcher::CreateMessageTemplateFromInteraction(const SleepyDiscord::Interaction & interaction)const
{
    SleepyDiscord::Message message;
    message.author = interaction.member.user;
    message.ID = interaction.message.ID;
    message.channelID = interaction.channelID;
    message.serverID = interaction.serverID;
    return message;
}

}
}
