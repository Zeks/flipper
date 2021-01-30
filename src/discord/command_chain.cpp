#include "discord/command_chain.h"
namespace discord{

void CommandChain::Push(Command&& command)
{
    commandTypes.push_back(command.type);
    commands.emplace_back(std::move(command));
}

void CommandChain::PushFront(Command&& command)
{
    commands.push_front(command);
}

void CommandChain::AddUserToCommands(QSharedPointer<User> user)
{
    for(auto& command : commands)
        command.user = user;
}

Command CommandChain::Pop()
{
    auto command = commands.back();
    commands.pop_back();
    return command;
}
}
