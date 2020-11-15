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
#include "discord/command_generators.h"

template <typename T>
struct TypeStringHolder{
    typedef T Type;
};


template <>
struct TypeStringHolder<discord::NextPageCommand>{
    static constexpr std::string_view name = "next";
        static constexpr std::string_view prefixlessPattern = "?<next>next";
        static constexpr std::string_view pattern = "next";
        static constexpr std::string_view help = "`%1next` to navigate to the next page";
        static constexpr std::string_view tips = "You can navigate through recommendations with `%1next`,`%1prev` or clicking on emoji";
        static constexpr std::string_view shorthand = "`{0}next`- displays next page of recommendations";
};
template <>
struct TypeStringHolder<discord::PreviousPageCommand>{
    static constexpr std::string_view name = "prev";
        static constexpr std::string_view prefixlessPattern = "?<prev>prev";
        static constexpr std::string_view pattern = "prev";
        static constexpr std::string_view help = "`%1prev` to navigate to the previous page";
        static constexpr std::string_view tips = "You can navigate through recommendations with `%1next`,`%1prev` or clicking on emoji";
        static constexpr std::string_view shorthand = "`{0}prev`- displays previous page of recommendations";
};

template <>
struct TypeStringHolder<discord::RecsCreationCommand> {
        static constexpr std::string_view name = "recs";
        static constexpr std::string_view prefixlessPattern = "?<recs>recs";
        static constexpr std::string_view pattern = "recs(\\s{1,}>refresh){0,1}((\\s{1,}\\d{4,10})|(\\s{1,}https:.{1}.{1}www.fanfiction.net.{1}.{1,100}))";
        static constexpr std::string_view help = "Basic commands:\n`%1recs USER_FFN_ID(or an url to your user page)` to create recommendations."
                                                 "\n`%1recs >refresh FFN_ID` if you want the bot to re-read your favourites.";

        static constexpr std::string_view modularHelp = "`%1recs FFN_ID` to create recommendations. FFN_ID is the id of your fanfiction.net profile"
                                                 "\n`%1recs >refresh FFN_ID` if you've added new stuff to your favourites and want the bot to re-read your list.";
        static constexpr std::string_view tips = "if you've added more fics to your favourites, refresh with the bot using `%1recs >refresh ffn_id`";
        static constexpr std::string_view shorthand = "`{0}recs`- displays recommendations ";
};

template <>
struct TypeStringHolder<discord::PageChangeCommand>{
        static constexpr std::string_view name = "page";
        static constexpr std::string_view prefixlessPattern = "?<page>page";
        static constexpr std::string_view pattern = "page\\s{1,}(\\d{1,10})";
        static constexpr std::string_view help = "`%1page X` to navigate to a differnt page in recommendation results. `%1page` without a number will repost the last one.";
        static constexpr std::string_view tips = "You can navigate to an exact page of recommendations with `%1page X`";
        static constexpr std::string_view shorthand = "`{0}page` - displays exact page of recommendations";
};
template <>
struct TypeStringHolder<discord::SetFandomCommand>{
        static constexpr std::string_view name = "fandom";
        static constexpr std::string_view prefixlessPattern = "?<fandom>fandom";
        static constexpr std::string_view pattern = "fandom(\\s{1,}>pure){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}";
        static constexpr std::string_view help = "\nFandom filter commands:\n`%1fandom X` for single fandom searches"
                                                 "\n`%1fandom >pure X` if you want to exclude crossovers "
                                                 "\n`%1fandom` a second time with a diffent fandom if you want to search for exact crossover"
                                                 "\n`%1fandom >reset` to reset fandom filter";
        static constexpr std::string_view tips = "You can get recs from a specific fandom with `%1fandom fandom name`";
        static constexpr std::string_view shorthand = "`{0}fandom` - shows fics from specific fandom";

};
template <>
struct TypeStringHolder<discord::IgnoreFandomCommand>{
        static constexpr std::string_view name = "xfandom";
        static constexpr std::string_view prefixlessPattern = "?<xfandom>xfandom";
        static constexpr std::string_view pattern = "xfandom(\\s{1,}>full){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}";
        static constexpr std::string_view help = "\nFandom ignore commands:\n`%1xfandom X` to permanently ignore fics just from this fandom or remove an ignore"
                                                 "\n`%1xfandom >full X` to also ignore crossovers from this fandom,"
                                                 "\n`%1xfandom >reset` to reset fandom ignore list";
        static constexpr std::string_view tips = "You can ignore fics from a specific fandom with `%1xfandom fandom name` or '%1xfandom >full fandom name' to also remove crossovers";
        static constexpr std::string_view shorthand = "`{0}xfandom` - ignores fics from a fandom";
};
template <>
struct TypeStringHolder<discord::IgnoreFicCommand>{
        static constexpr std::string_view name = "xfic";
        static constexpr std::string_view prefixlessPattern = "?<xfic>xfic";
        static constexpr std::string_view pattern = "xfic((\\s{1,}\\d{1,2}){1,10})|(\\s{1,}>all)";
        static constexpr std::string_view patternCommand = "xfic\\s{1,}(all){1,}";
        static constexpr std::string_view patternNum = "(?<silent>>silent\\s){0,1}(?<ids>\\d{1,2}\\s?)";
        static constexpr std::string_view help = "\nFanfic commands:\n`%1xfic X` will ignore a fic (you need input position in the last output), X Y Z to ignore multiple\n"
                                                 "Add >silent if you don't want the bot to repost the list (xfic >silent 1 2 ...)"
                                                 "\n`%1xfic all` will ignore the whole page";
                                                 //"\n`%1xfic >reset` resets the fic ignores";
        static constexpr std::string_view tips = "You can exclude fics from appearing in your lists with `%1xfic X` where X is a fic ID in the results.";
        static constexpr std::string_view shorthand = "`{0}xfic` - hides individual fics from further display";
};
template <>
struct TypeStringHolder<discord::DisplayHelpCommand>{
        static constexpr std::string_view name = "help";
        static constexpr std::string_view prefixlessPattern = "?<help>help";
        static constexpr std::string_view pattern = "help(\\s([A-Za-z]+)){0,1}";
        static constexpr std::string_view help = "`%1help` display this text";
        static constexpr std::string_view tips = "Enter `%1help` to see full help.";
};
template <>
struct TypeStringHolder<discord::RngCommand>{
        static constexpr std::string_view name = "roll";
        static constexpr std::string_view prefixlessPattern = "?<roll>roll";
        static constexpr std::string_view pattern = "roll\\s(best|good|all)";
        static constexpr std::string_view help = "`\n%1roll best/good/all` will display a set of 3 random fics from within a selected range in the recommendations.";
        static constexpr std::string_view tips = "You can roll fics from your recommendations gacha style using `%1roll best` (or `%1roll good` or `%1roll all`).";
        static constexpr std::string_view shorthand = "`{0}roll` - rolls fics from your recommendations gacha style";
};
template <>
struct TypeStringHolder<discord::ChangeServerPrefixCommand>{
        static constexpr std::string_view name = "prefix";
        static constexpr std::string_view prefixlessPattern = "?<pref>prefix";
        static constexpr std::string_view pattern = "prefix(\\s.+)";
        static constexpr std::string_view help = "\nBot management commands:\n`%1prefix new prefix` changes the comamnd prefix for this server(admin only)";
        static constexpr std::string_view shorthand = "{0}prefix so";
};

template <>
struct TypeStringHolder<discord::ChangePermittedChannelCommand>{
        static constexpr std::string_view name = "permit";
        static constexpr std::string_view prefixlessPattern = "?<pref>permit";
        static constexpr std::string_view pattern = "permit";
        static constexpr std::string_view help = "\nBot management commands:\n`%permit` sets the bot to work in this channel only (admin only)";
        static constexpr std::string_view shorthand = "{0}permit";
};



//template <>
//struct TypeStringHolder<discord::ForceListParamsCommand>{
//        static constexpr std::string_view name = "force";
//        static constexpr std::string_view prefixlessPattern = "?<force>force";
//        static constexpr std::string_view pattern = "force(\\s+\\d{1,2})(\\s+\\d{1,2})";
//        static constexpr std::string_view help = "";
//};

template <>
struct TypeStringHolder<discord::FilterLikedAuthorsCommand>{
        static constexpr std::string_view name = "liked";
        static constexpr std::string_view prefixlessPattern = "?<liked>liked";
        static constexpr std::string_view pattern = "liked";
        static constexpr std::string_view help = "\nFanfic filter commands:\n`%1liked` will enable a filter to show only fics from the authors whose fics you've liked in the list. Repeat to disable.";
        static constexpr std::string_view tips = "You can display only fics from the authors whose fics you have liked already with `%1liked`";
        static constexpr std::string_view shorthand = "`{0}liked` - shows only fics from the authors whose fics you've liked";
};

//template <>
//struct TypeStringHolder<discord::ShowFullFavouritesCommand>{
//        static constexpr std::string_view name = "favs";
//        static constexpr std::string_view prefixlessPattern = "?<favs>favs";
//        static constexpr std::string_view pattern = "favs";
//        static constexpr std::string_view help = "";
//};


template <>
struct TypeStringHolder<discord::ShowFreshRecsCommand>{
        static constexpr std::string_view name = "fresh";
        static constexpr std::string_view prefixlessPattern = "?<fresh>fresh";
        static constexpr std::string_view pattern = "fresh(?<strict>\\s>strict)";
        static constexpr std::string_view help = "`%1fresh` Will toggle sorting on published date to see fresh recommendations. Add >strict to display only recommendations with score>1";
        static constexpr std::string_view tips = "You can display fresh recommendations recently posted on FFN with `%1fresh` or `%1fresh >strict`.Strict version removes everything that doesn't have at least two votes. ";
        static constexpr std::string_view shorthand = "`{0}fresh` - displays fresh recommendations instead of top ones";
};

template <>
struct TypeStringHolder<discord::ShowCompletedCommand>{
        static constexpr std::string_view name = "complete";
        static constexpr std::string_view prefixlessPattern = "?<complete>complete";
        static constexpr std::string_view pattern = "complete";
        static constexpr std::string_view help = "`%1complete` Will filter out all incomplete fics.";
        static constexpr std::string_view tips = "You can display only completed fics with `%1complete`";
        static constexpr std::string_view shorthand = "`{0}complete` - shows only complete fics";
};

template <>
struct TypeStringHolder<discord::HideDeadCommand>{
        static constexpr std::string_view name = "dead";
        static constexpr std::string_view prefixlessPattern = "?<dead>dead";
        static constexpr std::string_view pattern = "dead(\\s{0,}\\d+)";
        static constexpr std::string_view help = "`%1dead` Will filter out all dead fics (not updated in a year),alternatively you can use it as `%1dead X` to set the \"death\" interval in days";
        static constexpr std::string_view tips = "You can remove fics not updated within a certain interval with `%1dead` or `%1dead X`";
        static constexpr std::string_view shorthand = "`{0}dead` - hides dead fics";
};

template <>
struct TypeStringHolder<discord::PurgeCommand>{
        static constexpr std::string_view name = "purge";
        static constexpr std::string_view prefixlessPattern = "?<purge>purge";
        static constexpr std::string_view pattern = "xxxpurge";
        static constexpr std::string_view help = "`%1purge` will purge ALL your data from the bot.";
        static constexpr std::string_view tips = "If you want the bot to reset all your data post `%1purge` command";
        static constexpr std::string_view shorthand = "`{0}purge`";
};

template <>
struct TypeStringHolder<discord::ResetFiltersCommand>{
        static constexpr std::string_view name = "xfilter";
        static constexpr std::string_view prefixlessPattern = "?<xfilter>xfilter";
        static constexpr std::string_view pattern = "xfilter";
        static constexpr std::string_view help = "`%1xfilter` will reset all active filters.";
        static constexpr std::string_view tips = "If you want to reset all active filters use`%1xfilter` command";
};

template <>
struct TypeStringHolder<discord::SimilarFicsCommand>{
        static constexpr std::string_view name = "similar";
        static constexpr std::string_view prefixlessPattern = "?<similar>similar";
        static constexpr std::string_view pattern = "similar(\\s{1,}\\d{1,15})";
        static constexpr std::string_view help = "`%1similar` will show fics most favourited with the provided FFN fic id.";
        static constexpr std::string_view tips = "`%1similar X` command doesn't necessarily display similar fics, just the ones most favourited with the provided one.";
        static constexpr std::string_view shorthand = "`{0}similar` - displays similar fics to other fics";
};

template <>
struct TypeStringHolder<discord::WordcountCommand>{
        static constexpr std::string_view name = "words";
        static constexpr std::string_view prefixlessPattern = "?<words>words";
        static constexpr std::string_view pattern = "words(\\s{1,}>{0,}\\s{0,}(less|more|between)(\\s{1,}\\d{1,5})(\\s{1,}\\d{1,5}){0,1}){0,1}";
        static constexpr std::string_view help = "`%1words` Allows filtering on fic size. Options are:\n`%1words >less X`\n`%1words >more X`\n`%1words >words X Y`\nWhere X and Y are lenghts in thousands of words.";
        static constexpr std::string_view tips = "`%1words >less` command with added 'socomplete' filter allows separating smaller finished fics recommended for you from larger ones which otherwise tend to get higher score based on traction alone.";
        static constexpr std::string_view shorthand = "`{0}words` - limits the size of the fics displayed";
};

template <>
struct TypeStringHolder<discord::ChangeTargetCommand>{
        static constexpr std::string_view name = "target";
        static constexpr std::string_view prefixlessPattern = "?<target>target";
        static constexpr std::string_view pattern = "target\\s(\\d+)";
        static constexpr std::string_view help = "";
        static constexpr std::string_view tips = "";
        static constexpr std::string_view shorthand = "";
};

template <>
struct TypeStringHolder<discord::SendMessageToChannelCommand>{
        static constexpr std::string_view name = "send";
        static constexpr std::string_view prefixlessPattern = "?<send>send";
        static constexpr std::string_view pattern = "send\\s(.+)";
        static constexpr std::string_view help = "";
        static constexpr std::string_view tips = "";
        static constexpr std::string_view shorthand = "";
};

template <>
struct TypeStringHolder<discord::ToggleBanCommand>{
        static constexpr std::string_view name = "ban";
        static constexpr std::string_view prefixlessPattern = "?<ban>ban";
        static constexpr std::string_view pattern = "ban\\s([A-Za-z]+)\\s(\\d+)";
        static constexpr std::string_view help = "";
        static constexpr std::string_view tips = "";
        static constexpr std::string_view shorthand = "";
};

