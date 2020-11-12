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
        RegisterCommand<ChangePermittedChannelCommand>(parser);
        //RegisterCommand<ForceListParamsCommand>(parser);
        RegisterCommand<FilterLikedAuthorsCommand>(parser);
        RegisterCommand<ShowFreshRecsCommand>(parser);
        RegisterCommand<ShowCompletedCommand>(parser);
        RegisterCommand<HideDeadCommand>(parser);
        RegisterCommand<PurgeCommand>(parser);
        RegisterCommand<ResetFiltersCommand>(parser);
        //RegisterCommand<SimilarFicsCommand>(parser);
        RegisterCommand<WordcountCommand>(parser);
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
        helpString +=  GetTipsForCommandIfActive<SimilarFicsCommand>();
        helpString +=  GetTipsForCommandIfActive<WordcountCommand>();
        helpString +=  QStringLiteral("If the genres you see in the list have + = and ~ next to them, they are auto-deduced.")+
                       QStringLiteral("\n+ means main genre")+
                       QStringLiteral("\n= means there's a sufficient amount of this genre in the fic.")+
                       QStringLiteral("\n~ means there's only a tiny bit of it.");
        helpString +=  QStringLiteral("Auto-deduced genres often differ from the genres the author has set, but they are typically more correct.");
        helpString +=  QStringLiteral("Fic's score is a representation of how much it appearared in favourite lists of other users' favourites selected for you, their closeness to yours and how closely the genres they read align to yours.");
        helpString +=  QStringLiteral("Your recommendations will be better if you remove fics you no longer consider good and fandoms you no longer read from your favourites.");
        helpString +=  QStringLiteral("Your recommendations are created by merging favourites of other users on ffn who have a bunch of favourite fics in common with you.");
        helpString +=  QStringLiteral("Socrates' fic database is updated approximately once a month by fetching new stuff from favourite lists.As such it only knows of about 40% (most relevant) of fics on fanfiction.net ");
        helpString +=  QStringLiteral("Once your favourite list has been fetched, further `%1recs` commands will use the cached version of the page and will be fast.");
        SendMessageCommand::tips = helpString;
    }

}



