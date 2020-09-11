#pragma once
#include "discord/task_environment.h"
#include "discord/command.h"



//ct_none = 0,
//ct_fill_recommendations = 1,
//ct_display_page = 2,
//ct_ignore_fics = 4,
//ct_list = 5,
//ct_tag = 6,
//ct_set_identity = 7,
//ct_set_fandoms = 8,
//ct_ignore_fandoms = 9,
//ct_display_help = 10

namespace discord {
class SendMessageCommand;
class ActionBase{
public:
    ActionBase(){}
    virtual ~ActionBase(){}
    virtual QSharedPointer<SendMessageCommand> Execute(QSharedPointer<TaskEnvironment>, Command);
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command) = 0;
    QSharedPointer<SendMessageCommand> action;
};



class HelpAction : public ActionBase{
public:
    HelpAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class RecsCreationAction : public ActionBase{
public:
    RecsCreationAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class DisplayPageAction : public ActionBase{
public:
    DisplayPageAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class DisplayRngAction : public ActionBase{
public:
    DisplayRngAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class SetFandomAction : public ActionBase{
public:
    SetFandomAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class IgnoreFandomAction : public ActionBase{
public:
    IgnoreFandomAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class IgnoreFicAction : public ActionBase{
public:
    IgnoreFicAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class TimeoutActiveAction : public ActionBase{
public:
    TimeoutActiveAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class NoUserInformationAction : public ActionBase{
public:
    NoUserInformationAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};
class ChangePrefixAction : public ActionBase{
public:
    ChangePrefixAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

class InsufficientPermissionsAction : public ActionBase{
public:
    InsufficientPermissionsAction(){}
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command);
};

struct ActionChain{
    void Clear(){actions.clear();performedParseCommand = false;}
    int Size(){return actions.size();}
    void Push(QSharedPointer<SendMessageCommand>);
    QSharedPointer<SendMessageCommand> Pop();
    ActionChain& operator+=(const ActionChain& other){
        this->actions += other.actions;
        return *this;
    };
    QQueue<QSharedPointer<SendMessageCommand>> actions;
    bool performedParseCommand = false;
};

QSharedPointer<ActionBase> GetAction(Command::ECommandType);


}
