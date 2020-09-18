#include "discord/discord_init.h"
#include "discord/command_creators.h"
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
        // this command requires actually implementing full mobile parser
        // RegisterCommand<ShowFullFavouritesCommand>(parser);
    }

    template<typename T> QString GetRegexForCommandIfActive(){
        QString result;
        if(CommandState<T>::active)
            result = CommandState<T>::regexCommandIdentifier;
        return result;
    }
}



