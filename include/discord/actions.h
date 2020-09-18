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

#define ACTION(X) class X : public ActionBase{ \
public: \
    X(){} \
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command); \
}

ACTION(HelpAction);
ACTION(RecsCreationAction);
ACTION(DisplayPageAction);
ACTION(DisplayRngAction);
ACTION(SetFandomAction);
ACTION(IgnoreFandomAction);
ACTION(IgnoreFicAction);
ACTION(TimeoutActiveAction);
ACTION(NoUserInformationAction);
ACTION(ChangePrefixAction);
ACTION(SetForcedListParamsAction);
ACTION(SetForceLikedAuthorsAction);
ACTION(InsufficientPermissionsAction);
ACTION(NullAction);
ACTION(ShowFullFavouritesAction);
ACTION(ShowFreshRecommendationsAction);
ACTION(ShowCompleteAction);
ACTION(HideDeadAction);

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
