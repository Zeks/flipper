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
        // this command requires actually implementing full mobile parser
        // RegisterCommand<ShowFullFavouritesCommand>(parser);
    }

    template<typename T> QString GetRegexForCommandIfActive(){
        QString result;
        if(CommandState<T>::active)
            result = CommandState<T>::regexCommandIdentifier;
        return result;
    }

    QString GetSimpleCommandIdentifierPrefixless(){
        CommandState<RecsCreationCommand>::regexCommandIdentifier= "recs";
        CommandState<NextPageCommand>::regexCommandIdentifier = "next";
        CommandState<PreviousPageCommand>::regexCommandIdentifier = "prev";
        CommandState<PageChangeCommand>::regexCommandIdentifier = "page";
        CommandState<SetFandomCommand>::regexCommandIdentifier = "fandom";
        CommandState<IgnoreFandomCommand>::regexCommandIdentifier = "xfandom";
        CommandState<IgnoreFicCommand>::regexCommandIdentifier = "xfic";
        CommandState<DisplayHelpCommand>::regexCommandIdentifier = "help";
        CommandState<RngCommand>::regexCommandIdentifier = "roll";

        QStringList list;
        list.push_back(GetRegexForCommandIfActive<RecsCreationCommand>());
        list.push_back(GetRegexForCommandIfActive<NextPageCommand>());
        list.push_back(GetRegexForCommandIfActive<PreviousPageCommand>());
        list.push_back(GetRegexForCommandIfActive<PageChangeCommand>());
        list.push_back(GetRegexForCommandIfActive<SetFandomCommand>());
        list.push_back(GetRegexForCommandIfActive<IgnoreFandomCommand>());
        list.push_back(GetRegexForCommandIfActive<IgnoreFicCommand>());
        list.push_back(GetRegexForCommandIfActive<DisplayHelpCommand>());
        list.push_back(GetRegexForCommandIfActive<RngCommand>());
        list.push_back(GetRegexForCommandIfActive<ChangeServerPrefixCommand>());
        list.push_back(GetRegexForCommandIfActive<ForceListParamsCommand>());
        list.push_back(GetRegexForCommandIfActive<FilterLikedAuthorsCommand>());
        list.removeAll("");
        return list.join("|");
    }

    void InitHelpForCommands(){
        QString helpString = "Basic commands:\n`!recs FFN_ID` to create recommendations";
        CommandState<NextPageCommand>::help = "`!next` to navigate to the next page of the recommendation results";
        CommandState<PreviousPageCommand>::help = "`!prev` to navigate to the previous page of the recommendation results";
        CommandState<PageChangeCommand>::help = "`!page X` to navigate to a differnt page in recommendation results";
        CommandState<SetFandomCommand>::help = "\nFandom filter commands:\n`!fandom X` for single fandom searches"
                                               "\n`!fandom >pure X` if you want to exclude crossovers "
                                               "\n`!fandom` a second time with a diffent fandom if you want to search for exact crossover"
                                               "\n`!fandom >reset` to reset fandom filter";
        CommandState<IgnoreFandomCommand>::help = "\nFandom ignore commands:\n`!xfandom X` to permanently ignore fics just from this fandom or remove an ignore"
                                                  "\n`!xfandom >full X` to also ignore crossovers from this fandom,"
                                                  "\n`!xfandom >reset` to reset fandom ignore list";
        //CommandState<IgnoreFandomWithCrossesCommand>::help = "xcrossfandom to permanently ignore a fandom eve when it appears in crossovers, repeat to unignore";
        CommandState<IgnoreFicCommand>::help = "\nFanfic commands:\n`!xfic X` will ignore a fic (you need input position in the last output), X Y Z to ignore multiple"
                                               "\n`!xfic >all` will ignore the whole page";
                                               //"\n`!xfic >reset` resets the fic ignores";
        CommandState<RngCommand>::help = "`\n!roll best/good/all` will display a set of 3 random fics from within a selected range in the recommendations.";
        CommandState<DisplayHelpCommand>::help = "`!help` display this text";
        CommandState<DisplayHelpCommand>::help = "\nBot management commands:\n`!prefix new prefix` changes the comamnd prefix for this server(admin only)";
    }

    void InitPrefixlessRegularExpressions()
    {
        CommandState<RecsCreationCommand>::regexWithoutPrefix= "recs\\s{1,}(\\d{4,10})";
        CommandState<NextPageCommand>::regexWithoutPrefix = "next";
        CommandState<PreviousPageCommand>::regexWithoutPrefix = "prev";
        CommandState<PageChangeCommand>::regexWithoutPrefix = "page\\s{1,}(\\d{1,10})";
        CommandState<SetFandomCommand>::regexWithoutPrefix = "fandom(\\s{1,}>pure){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}";
        CommandState<IgnoreFandomCommand>::regexWithoutPrefix = "xfandom(\\s{1,}>full){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}";
        CommandState<IgnoreFicCommand>::regexWithoutPrefix = "xfic((\\s{1,}\\d{1,2}){1,10})|(\\s{1,}>all)";
        CommandState<DisplayHelpCommand>::regexWithoutPrefix = "help";
        CommandState<RngCommand>::regexWithoutPrefix = "roll\\s(best|good|all)";
        CommandState<ChangeServerPrefixCommand>::regexWithoutPrefix = "prefix(\\s.+)";
    }


}



