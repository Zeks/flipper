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
        RegisterCommand<DisplayHelpCommand>(parser);
        RegisterCommand<RngCommand>(parser);
        RegisterCommand<StatsCommand>(parser);
        RegisterCommand<GemsCommand>(parser);
        RegisterCommand<ChangeServerPrefixCommand>(parser);
        RegisterCommand<ChangePermittedChannelCommand>(parser);
        RegisterCommand<FilterLikedAuthorsCommand>(parser);
        RegisterCommand<ShowFreshRecsCommand>(parser);
        RegisterCommand<ShowCompletedCommand>(parser);
        RegisterCommand<HideDeadCommand>(parser);
        RegisterCommand<SusCommand>(parser);
        RegisterCommand<PurgeCommand>(parser);
        RegisterCommand<ChangeTargetCommand>(parser);
        RegisterCommand<SendMessageToChannelCommand>(parser);
        RegisterCommand<ToggleBanCommand>(parser);
        RegisterCommand<ResetFiltersCommand>(parser);
        RegisterCommand<WordcountCommand>(parser);
        RegisterCommand<CutoffCommand>(parser);
        RegisterCommand<YearCommand>(parser);
        RegisterCommand<ShowCommand>(parser);
        RegisterCommand<ReviewCommand>(parser);
        //RegisterCommand<ForceListParamsCommand>(parser);
        //RegisterCommand<SetIdentityCommand>(parser);
        //RegisterCommand<SimilarFicsCommand>(parser);
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
        helpString +=  GetTipsForCommandIfActive<YearCommand>();
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





