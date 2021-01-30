#pragma once
#include "discord/command.h"
namespace discord {
struct CommandChain{
    CommandChain() = default;
    CommandChain(CommandChain&&) = default;
    CommandChain& operator=(CommandChain&& other) = default;
    CommandChain(const CommandChain&) = delete;

    int Size(){return commands.size();}
    void Push(Command&&);
    void PushFront(Command&&);
    void AddUserToCommands(QSharedPointer<User>);
    Command Pop();

    CommandChain& operator+=(CommandChain& other){
        for(auto&& item : other.commands){
            this->commands.emplace_back(std::move(item));
        }
        for(auto&& item : other.commandTypes){
            this->commandTypes.emplace_back(std::move(item));
        }

        return *this;
    };
    CommandChain& operator+=(CommandChain&& other){
        for(auto&& item : other.commands){
            this->commands.emplace_back(std::move(item));
        }
        for(auto&& item : other.commandTypes){
            this->commandTypes.emplace_back(std::move(item));
        }
        return *this;
    };
    void Reset(){
        commands.clear();
        commandTypes.clear();
        hasParseCommand = false;
        stopExecution = false;
    };
    std::list<Command> commands;
    std::vector<ECommandType> commandTypes;
    QSharedPointer<User> user;
    bool hasParseCommand = false;
    bool hasFullParseCommand = false;
    bool stopExecution = false;
    int delayMs = 0;
};

}
