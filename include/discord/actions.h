/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include "discord/task_environment.h"
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/command.h"
//#include "discord/



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
    virtual QSharedPointer<SendMessageCommand> Execute(QSharedPointer<TaskEnvironment>, Command&&);
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&&) = 0;
    std::shared_ptr<TrackedMessageBase> messageData;
    QSharedPointer<SendMessageCommand> action;
};

#define ACTION(X) class X : public ActionBase{ \
public: \
    X(){} \
    virtual QSharedPointer<SendMessageCommand> ExecuteImpl(QSharedPointer<TaskEnvironment>, Command&&); \
}

ACTION(GeneralHelpAction);
//ACTION(HelpAction);
ACTION(DesktopRecsCreationAction);
ACTION(MobileRecsCreationAction);
ACTION(DisplayPleaAction);
ACTION(DisplayPageAction);
ACTION(DisplayRngAction);
ACTION(SetFandomAction);
ACTION(IgnoreFandomAction);
ACTION(IgnoreFicAction);
ACTION(TimeoutActiveAction);
ACTION(NoUserInformationAction);
ACTION(ChangePrefixAction);
ACTION(SetChannelAction);
ACTION(SetForcedListParamsAction);
ACTION(SetForceLikedAuthorsAction);
ACTION(InsufficientPermissionsAction);
ACTION(NullAction);
ACTION(ShowFullFavouritesAction);
ACTION(ShowFreshRecommendationsAction);
ACTION(ShowGemsAction);
ACTION(CutoffAction);
ACTION(YearAction);
ACTION(ShowFicAction);
ACTION(ShowCompleteAction);
ACTION(HideDeadAction);
ACTION(PurgeAction);
ACTION(RemoveReactions);
ACTION(ResetFiltersAction);
ACTION(CreateSimilarFicListAction);
ACTION(SetWordcountLimitAction);
ACTION(SetTargetChannelAction);
ACTION(SendMessageToChannelAction);
ACTION(ToggleBanAction);
ACTION(SusAction);
ACTION(AddReviewAction); // also edit
ACTION(SpawnRemoveConfirmationAction);
ACTION(DeleteEntityAction);
ACTION(DeleteBotMessageAction);
ACTION(DisplayReviewAction);
ACTION(RemoveMessageTextAction);
ACTION(ToggleFunctionalityForServerAction);


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
    bool performedFullParseCommand = false;
};

QSharedPointer<ActionBase> GetAction(ECommandType);


}
