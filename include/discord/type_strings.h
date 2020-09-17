#pragma once
#include "discord/command_creators.h"

template <typename T>
struct TypeStringHolder{
    typedef T Type;
};

template <>
struct TypeStringHolder<discord::RecsCreationCommand> {
        static constexpr std::string_view name = "recs";
        static constexpr std::string_view prefixlessPattern = "?<recs>recs";
        static constexpr std::string_view pattern = "recs\\s{1,}(\\d{4,10})";
        static constexpr std::string_view help = "Basic commands:\n`%1recs FFN_ID` to create recommendations. FFN_ID is the id of your fanfiction.net profile";
};
template <>
struct TypeStringHolder<discord::NextPageCommand>{
    static constexpr std::string_view name = "next";
        static constexpr std::string_view prefixlessPattern = "?<next>next";
        static constexpr std::string_view pattern = "next";
        static constexpr std::string_view help = "`%1next` to navigate to the next page of the recommendation results";
};
template <>
struct TypeStringHolder<discord::PreviousPageCommand>{
    static constexpr std::string_view name = "prev";
        static constexpr std::string_view prefixlessPattern = "?<prev>prev";
        static constexpr std::string_view pattern = "prev";
        static constexpr std::string_view help = "`%1prev` to navigate to the previous page of the recommendation results";
};
template <>
struct TypeStringHolder<discord::PageChangeCommand>{
        static constexpr std::string_view name = "page";
        static constexpr std::string_view prefixlessPattern = "?<page>page";
        static constexpr std::string_view pattern = "page\\s{1,}(\\d{1,10})";
        static constexpr std::string_view help = "`%1page X` to navigate to a differnt page in recommendation results";
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
};
template <>
struct TypeStringHolder<discord::IgnoreFandomCommand>{
        static constexpr std::string_view name = "xfandom";
        static constexpr std::string_view prefixlessPattern = "?<xfandom>xfandom";
        static constexpr std::string_view pattern = "xfandom(\\s{1,}>full){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}";
        static constexpr std::string_view help = "\nFandom ignore commands:\n`%1xfandom X` to permanently ignore fics just from this fandom or remove an ignore"
                                                 "\n`%1xfandom >full X` to also ignore crossovers from this fandom,"
                                                 "\n`%1xfandom >reset` to reset fandom ignore list";
};
template <>
struct TypeStringHolder<discord::IgnoreFicCommand>{
        static constexpr std::string_view name = "xfic";
        static constexpr std::string_view prefixlessPattern = "?<xfic>xfic";
        static constexpr std::string_view pattern = "xfic((\\s{1,}\\d{1,2}){1,10})|(\\s{1,}>all)";
        static constexpr std::string_view patternCommand = "xfic\\s{1,}(>all){1,}";
        static constexpr std::string_view patternNum = "(?<silent>>silent\\s){0,1}(?<ids>\\d{1,2}\\s?)";
        static constexpr std::string_view help = "\nFanfic commands:\n`%1xfic X` will ignore a fic (you need input position in the last output), X Y Z to ignore multiple\n"
                                                 "Add >silent if you don't want the bot to repost the list (xfic >silent 1 2 ...)"
                                                 "\n`%1xfic >all` will ignore the whole page";
                                                 //"\n`%1xfic >reset` resets the fic ignores";
};
template <>
struct TypeStringHolder<discord::DisplayHelpCommand>{
        static constexpr std::string_view name = "help";
        static constexpr std::string_view prefixlessPattern = "?<help>help";
        static constexpr std::string_view pattern = "help";
        static constexpr std::string_view help = "`%1help` display this text";
};
template <>
struct TypeStringHolder<discord::RngCommand>{
        static constexpr std::string_view name = "roll";
        static constexpr std::string_view prefixlessPattern = "?<roll>roll";
        static constexpr std::string_view pattern = "roll\\s(best|good|all)";
        static constexpr std::string_view help = "`\n%1roll best/good/all` will display a set of 3 random fics from within a selected range in the recommendations.";
};
template <>
struct TypeStringHolder<discord::ChangeServerPrefixCommand>{
        static constexpr std::string_view name = "prefix";
        static constexpr std::string_view prefixlessPattern = "?<pref>prefix";
        static constexpr std::string_view pattern = "prefix(\\s.+)";
        static constexpr std::string_view help = "\nBot management commands:\n`%1prefix new prefix` changes the comamnd prefix for this server(admin only)";
};

template <>
struct TypeStringHolder<discord::ForceListParamsCommand>{
        static constexpr std::string_view name = "force";
        static constexpr std::string_view prefixlessPattern = "?<force>force";
        static constexpr std::string_view pattern = "force(\\s\\d{1,2})(\\s\\d{1,2})";
        static constexpr std::string_view help = "";
};

template <>
struct TypeStringHolder<discord::FilterLikedAuthorsCommand>{
        static constexpr std::string_view name = "liked";
        static constexpr std::string_view prefixlessPattern = "?<liked>liked";
        static constexpr std::string_view pattern = "liked";
        static constexpr std::string_view help = "\nFanfic filter commands:\n`%1liked` will enable a filter to show only fics from the authors whose fics you've liked in the list. Repeat to disable.";
};

template <>
struct TypeStringHolder<discord::ShowFullFavouritesCommand>{
        static constexpr std::string_view name = "favs";
        static constexpr std::string_view prefixlessPattern = "?<favs>favs";
        static constexpr std::string_view pattern = "favs";
        static constexpr std::string_view help = "";
};


template <>
struct TypeStringHolder<discord::ShowFreshRecsCommand>{
        static constexpr std::string_view name = "fresh";
        static constexpr std::string_view prefixlessPattern = "?<fresh>fresh";
        static constexpr std::string_view pattern = "fresh(?<strict>\\s>strict)";
        static constexpr std::string_view help = "`%1fresh` Will toggle sorting on published date to see fresh recommendations. Add >strict to display only recommendations with score>1";
};




