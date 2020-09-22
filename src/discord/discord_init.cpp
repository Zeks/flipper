#include "discord/discord_init.h"
#include "discord/command_generators.h"
#include "discord/actions.h"
#include "discord/type_strings.h"
#include <regex>
namespace discord {

    void InitDefaultCommandSet(QSharedPointer<CommandParser> parser)
    {
        RegisterCommand<RecsCreationCommand>(parser);
        RegisterCommand<PageChangeCommand>(parser);
        RegisterCommand<NextPageCommand>(parser);
        RegisterCommand<PreviousPageCommand>(parser);
        RegisterCommand<SetFandomCommand>(parser);
        RegisterCommand<IgnoreFandomCommand>(parser);
        RegisterCommand<IgnoreFicCommand>(parser);
        //RegisterCommand<SetIdentityCommand>(parser);
        RegisterCommand<DisplayHelpCommand>(parser);
        RegisterCommand<RngCommand>(parser);
        RegisterCommand<ChangeServerPrefixCommand>(parser);
        RegisterCommand<ForceListParamsCommand>(parser);
        RegisterCommand<FilterLikedAuthorsCommand>(parser);
        RegisterCommand<ShowFreshRecsCommand>(parser);
        RegisterCommand<ShowCompletedCommand>(parser);
        RegisterCommand<HideDeadCommand>(parser);
        RegisterCommand<PurgeCommand>(parser);
        RegisterCommand<ResetFiltersCommand>(parser);
        // this command requires actually implementing full mobile parser
        // RegisterCommand<ShowFullFavouritesCommand>(parser);
    }

    template<typename T> QString GetTipsForCommandIfActive(){
        QString result;
        if(CommandState<T>::active)
            result = "\n" + QString::fromStdString(std::string(TypeStringHolder<T>::tips));
        return result;
    }

    void InitTips(){
        QStringList helpString;
        helpString +=  GetTipsForCommandIfActive<RecsCreationCommand>();
        helpString +=  GetTipsForCommandIfActive<NextPageCommand>();
        helpString +=  GetTipsForCommandIfActive<PageChangeCommand>();
        helpString +=  GetTipsForCommandIfActive<SetFandomCommand>();
        helpString +=  GetTipsForCommandIfActive<IgnoreFandomCommand>();
        helpString +=  GetTipsForCommandIfActive<IgnoreFicCommand>();
        helpString +=  GetTipsForCommandIfActive<DisplayHelpCommand>();
        helpString +=  GetTipsForCommandIfActive<RngCommand>();
        helpString +=  GetTipsForCommandIfActive<FilterLikedAuthorsCommand>();
        helpString +=  GetTipsForCommandIfActive<ShowFreshRecsCommand>();
        helpString +=  GetTipsForCommandIfActive<ShowCompletedCommand>();
        helpString +=  GetTipsForCommandIfActive<HideDeadCommand>();
        helpString +=  GetTipsForCommandIfActive<PurgeCommand>();
        helpString +=  GetTipsForCommandIfActive<ResetFiltersCommand>();
        helpString +=  "If you would like to support the bot, you can do it on https://www.patreon.com/Zekses";
        SendMessageCommand::tips = helpString;
    }

}



