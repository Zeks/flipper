#pragma once
#include "discord/command_chain.h"
#include "discord/command_generators.h"
namespace discord {
class CommandParser{
public:
    CommandChain Execute(const std::string &command, QSharedPointer<Server> server, const SleepyDiscord::Message &);
    QList<QSharedPointer<CommandCreator>> commandProcessors;
    Client* client = nullptr;
    std::mutex lock;
};
}
