#pragma once
#include "discord/command_creators.h"

#include <QSharedPointer>

namespace discord {
    class CommandParser;
    template <typename T>
    auto RegisterCommand(QSharedPointer<CommandParser> parser){
        parser->commandProcessors.push_back(QSharedPointer<T>(new T()));
        CommandState<T>::active = true;
    };
    void InitDefaultCommandSet(QSharedPointer<CommandParser> parser);
    void InitTips();
    void InitHelpForCommands();
    QString GetSimpleCommandIdentifierPrefixless();
    void InitPrefixlessRegularExpressions();
}
