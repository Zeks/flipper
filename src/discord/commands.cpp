#include "discord/commands.h"
#include "Interfaces/discord/users.h"
#include "include/grpc/grpc_source.h"
#include <QSettings>
namespace discord{

void CommandChain::Push(Command command)
{
    commands.push_back(command);
}

Command CommandChain::Pop()
{
    auto command = commands.last();
    commands.pop_back();
    return command;
}


CommandChain CommandCreator::ProcessInput(SleepyDiscord::Message message , bool verifyUser)
{
    if(verifyUser)
        EnsureUserExists(QString::fromStdString(message.author.ID), QString::fromStdString(message.author.username));

    match = rx.match(QString::fromStdString(message.content));
    if(!match.hasMatch())
    {
        result.Push(nullCommand);
        return result;
    }
    return ProcessInputImpl(message);
}

void CommandCreator::EnsureUserExists(QString userId, QString userName)
{
    An<Users> users;
    if(!users->HasUser(userId)){
        bool inDatabase = users->LoadUser(userId);
        if(!inDatabase)
        {
            QSharedPointer<discord::User> user(new discord::User(userId, "-1", userName));
            users->userInterface->WriteUser(user);
            users->LoadUser(userId);
        }
    }
}


RecsCreationCommand::RecsCreationCommand()
{

    rx = QRegularExpression("^!recs\\s(\\d{4,10})");
}



CommandChain RecsCreationCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command createRecs;
    createRecs.type = Command::ct_fill_recommendations;
    createRecs.ids.push_back(match.captured(1).toUInt());
    createRecs.originalMessage = message;
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(createRecs);
    result.Push(displayRecs);
    return result;
}



PageChangeCommand::PageChangeCommand()
{
    rx = QRegularExpression("^!page\\s(\\d{1,10})");
}

CommandChain PageChangeCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_change_page;
    command.ids.push_back(match.captured(1).toUInt());
    command.originalMessage = message;
    result.Push(command);
    return result;
}

NextPageCommand::NextPageCommand()
{
    rx = QRegularExpression("^!next");
}

CommandChain NextPageCommand::ProcessInputImpl(SleepyDiscord::Message)
{

}

PreviousPageCommand::PreviousPageCommand()
{
    rx = QRegularExpression("^!prev");
}

CommandChain PreviousPageCommand::ProcessInputImpl(SleepyDiscord::Message)
{

}


}
