#include "discord/help_generator.h"
#include "discord/type_strings.h"
#include "GlobalHeaders/snippets_templates.h"
#include "fmt/format.h"

namespace discord {



SleepyDiscord::Embed GetHelpPage(int pageNumber, std::string_view serverPrefix)
{
    switch(static_cast<EHelpPages>(pageNumber)){
    case EHelpPages::general_help:
        return GetTopLevelHelpPage(serverPrefix);
    case EHelpPages::list_navigation_help:
        return GetListNavigationPage(serverPrefix);
    case EHelpPages::list_modes_help:
        return GetListModesPage(serverPrefix);
    case EHelpPages::recs_help:
        return GetRecsHelpPage(serverPrefix);
    case EHelpPages::fresh_help:
        return GetFreshHelpPage(serverPrefix);
    case EHelpPages::roll_help:
        return GetRollHelpPage(serverPrefix);
    case EHelpPages::fanfic_filter_help:
        return GetFanficFiltersHelpPage(serverPrefix);
    case EHelpPages::fandom_filter_help:
        return GetFandomFiltersHelpPage(serverPrefix);

    default:
        return GetTopLevelHelpPage(serverPrefix);
    }
}

int GetHelpPage(std::string_view argument)
{
    EHelpPages result;

    if(argument * in("recs"))
        result = EHelpPages::recs_help;
    else if(argument * in("next", "prev", "page"))
        result = EHelpPages::list_navigation_help;
    else if(argument * in("fresh"))
        result = EHelpPages::fresh_help;
    else if(argument * in("roll"))
        result = EHelpPages::roll_help;
    else if(argument * in("complete", "dead", "liked", "xfic", "words"))
        result = EHelpPages::fanfic_filter_help;
    else if(argument * in("fandom", "xfandom"))
        result = EHelpPages::fandom_filter_help;
    else
        result = EHelpPages::general_help;
    return static_cast<int>(result);
}
SleepyDiscord::Embed GetTopLevelHelpPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;
    std::string header = "To start using the bot you will need to do `{0}recs X` where X is your profile id on fanfiction.net or url to that which you can get [here](https://www.fanfiction.net/account/settings.php)\n"
                     "\nYou will also need to fill favourites on your profile for the bot to recommend something to you."
                     "Once the results are displayed you can navigate them with the emoji underneath the message or `next`, `prev` and `page` commands.";
                     //"Additionally, you can use these commands to filter and change the displayed recommendations in various ways: ";
    header=fmt::format(header, serverPrefix);
    SleepyDiscord::EmbedField headerField;

    headerField.isInline = false;
    headerField.name = "Using the bot:";
    headerField.value = header;


    SleepyDiscord::EmbedField navigationField;
    std::string navigationText;
    navigationText += std::string(TypeStringHolder<PageChangeCommand>::shorthand) + "\n";
    navigationText += std::string(TypeStringHolder<NextPageCommand>::shorthand) + "\n";
    navigationText += std::string(TypeStringHolder<PreviousPageCommand>::shorthand) + "\n";
    navigationText=fmt::format(navigationText, serverPrefix);

    navigationField.isInline = true;
    navigationField.name = "Navigating the list:";
    navigationField.value = navigationText;


    SleepyDiscord::EmbedField fanficField;
    std::string fanficText;
    fanficText += std::string(TypeStringHolder<ShowCompletedCommand>::shorthand) + "\n";
    fanficText += std::string(TypeStringHolder<HideDeadCommand>::shorthand) + "\n";
    fanficText += std::string(TypeStringHolder<FilterLikedAuthorsCommand>::shorthand) + "\n";
    fanficText += std::string(TypeStringHolder<WordcountCommand>::shorthand) + "\n";
    fanficText += std::string(TypeStringHolder<IgnoreFicCommand>::shorthand);
    fanficText=fmt::format(fanficText, serverPrefix);
    fanficField.isInline = true;
    fanficField.name = "Filtering fanfics:";
    fanficField.value = fanficText;

    SleepyDiscord::EmbedField listTypesField;
    listTypesField.isInline = true;
    listTypesField.name = "Changing types of displayed list:";
    std::string listTypesText;
    listTypesText += std::string(TypeStringHolder<RecsCreationCommand>::shorthand) + "\n";
    listTypesText += std::string(TypeStringHolder<ShowFreshRecsCommand>::shorthand) + "\n";
    listTypesText += std::string(TypeStringHolder<RngCommand>::shorthand) + "\n";
    //listTypesText += std::string(TypeStringHolder<SimilarFicsCommand>::shorthand) + "\n";
    listTypesText=fmt::format(listTypesText, serverPrefix);
    listTypesField.value = listTypesText;

    SleepyDiscord::EmbedField fandomField;
    fandomField.isInline = true;
    fandomField.name = "Filtering fandoms:";
    std::string fandomText;
    fandomText += std::string(TypeStringHolder<SetFandomCommand>::shorthand) + "\n";
    fandomText += std::string(TypeStringHolder<IgnoreFandomCommand>::shorthand);
    fandomText=fmt::format(fandomText, serverPrefix);
    fandomField.value = fandomText;

    SleepyDiscord::EmbedField paddingField;
    paddingField.isInline = false;
    paddingField.name = "List filters:";
    std::string paddingText = "Use the next commands to apply filters to displayed recommendations."
                         "To get detailed help for each of them use `{0}help commandname`\n"
            "Most of them have additional optional parameters you might want to use."
            "\n\nTo reset any filter repeat its command without arguments or issue '{0}xfilter' command to reset everything at the same time.";
    paddingText=fmt::format(paddingText, serverPrefix);
    paddingField.value = paddingText;


    SleepyDiscord::EmbedField adminCommandsField;
    adminCommandsField.isInline = false;
    adminCommandsField.name = "Admin only commands:";
    std::string admintText = "`{0}prefix new_prefix` changes bot's command prefix. `.` can't be used in a prefix\n";
    admintText  +="`{0}permit` forces the bot to answer in this channel only";
    admintText=fmt::format(admintText, serverPrefix);
    adminCommandsField.value = admintText;

    SleepyDiscord::EmbedField supportField;
    supportField.isInline = false;
    supportField.name = "Support:";
    std::string supportText = "If you need help with the bot itself, join its [official server](https://discord.gg/dpAnunJ)\n";
    supportText  +="If you want to support the bot's development and hosting you can do it on [Patreon](https://www.patreon.com/zekses)";
    supportText=fmt::format(supportText, serverPrefix);

    supportField.value = supportText;

    embed.fields.push_back(headerField);
    embed.fields.push_back(navigationField);
    embed.fields.push_back(listTypesField);
    embed.fields.push_back(paddingField);
    embed.fields.push_back(fanficField);
    embed.fields.push_back(fandomField);
    embed.fields.push_back(adminCommandsField);
    embed.fields.push_back(supportField);

    SleepyDiscord::EmbedFooter footer;
    std::string footerText = "To reset all your data in the bot, issue `{0}purge`";
    footerText=fmt::format(footerText, serverPrefix);
    footer.text = footerText;
    embed.footer = footer;
    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetListNavigationPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;
    std::string header = "The size of generated recommendation list is typically pretty large and depending "
                         "on your favourites size can range into hundreds of thousands of fanfics arranged by relevance."
                         "\n\nTo navigate the recommendations you can use emoji underneath bot's posts to scroll pages or `{0}next`, `{0}prev` and `{0}page` commands.";

    header=fmt::format(header, serverPrefix);
    SleepyDiscord::EmbedField headerField;

    headerField.isInline = false;
    headerField.name = "List navigation overview:";
    headerField.value = header;

    SleepyDiscord::EmbedField nextField;
    nextField.isInline = true;
    nextField.name = "Next page:";
    std::string fieldText = "`{0}next` is used to navigate to the next page in recommedations.\n\nAlternatively you can click on 👉 emoji to do the same.";
    fieldText=fmt::format(fieldText, serverPrefix);
    nextField.value = fieldText;

    SleepyDiscord::EmbedField previousField;
    previousField.isInline = true;
    previousField.name = "Previous page:";
    fieldText = "`{0}prev` is used to navigate to the previous page in recommedations.\n\nAlternatively you can click on 👈 emoji to do the same.";
    fieldText=fmt::format(fieldText, serverPrefix);
    previousField.value = fieldText;


    SleepyDiscord::EmbedField exactField;
    exactField.isInline = false;
    exactField.name = "Selecting exact page:";
    fieldText = "`{0}page` can be used without parameters to respawn your recommendation list at the page you stopped."
                "\n`{0}page 2` will repost your recommendations at page 2 (change to any other page you need)";
    fieldText=fmt::format(fieldText, serverPrefix);
    exactField.value = fieldText;

    embed.fields.push_back(headerField);
    embed.fields.push_back(nextField);
    embed.fields.push_back(previousField);
    embed.fields.push_back(exactField);SleepyDiscord::EmbedField recsField;
    recsField.isInline = false;
    recsField.name = "Top recommendations mode.";
    fieldText = "`{0}recs` is used to generate recommendations. To do that you need to provide it with either your profile id on fanfiction.net"
                            " that you can get [here](https://www.fanfiction.net/account/settings.php) or custom url to your page."
                            "\n\nFor the command to work properly, your must have favourited some fics before calling this command."
                            "Remember that it takes a bit of time for fanfiction.net to update your page so make sure your favourites are displaying before calling {0}recs"
                            "\n\nExamples:"
                            "\n`{0}recs 111` calling command with profile ID"
                            "\n`{0}recs https://www.fanfiction.net/name` calling command with profile url";
    fieldText=fmt::format(fieldText, serverPrefix);
    recsField.value = fieldText;

//    SleepyDiscord::EmbedField previousField;
//    previousField.isInline = true;
//    previousField.name = "Previous page:";
//    fieldText = "`{0}prev` is used to navigate to the previous page in recommedations.\n\nAlternatively you can click on 👈 emoji to do the same.";
//    fieldText=fmt::format(fieldText, serverPrefix);
//    previousField.value = fieldText;


//    SleepyDiscord::EmbedField exactField;
//    exactField.isInline = false;
//    exactField.name = "Selecting exact page:";
//    fieldText = "`{0}page` can be used without parameters to respawn your recommendation list at the page you stopped."
//                "\n`{0}page 2` will repost your recommendations at page 2 (change to any other page you need)";
//    fieldText=fmt::format(fieldText, serverPrefix);
//    exactField.value = fieldText;

    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetListModesPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;
    std::string header = "There are four modes of list display you can try:"
                         "\n\n1. Top recommendations mode. Used by default and enabled with `{0}recs`"
                         "\n2. Fresh recommendations mode that can be enabled with `{0}fresh`"
                         "\n3. Similar fics display mode activated by `{0}similar`"
                         "\n4. Random rolls within recommendations activated by`{0}roll`"
                         "\n\nThese modes are exclusive and you need to switch between them with respective commands if you need something else than what is currently enabled."
                         "\nUse 👉 to see modes descriptions.";

    header=fmt::format(header, serverPrefix);
    SleepyDiscord::EmbedField headerField;

    headerField.isInline = false;
    headerField.name = "List navigation overview:";
    headerField.value = header;

    embed.fields.push_back(headerField);
    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetRecsHelpPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;

    SleepyDiscord::EmbedField recsField;
    recsField.isInline = false;
    recsField.name = "Top recommendations mode usage:";
    std::string fieldText = "`{0}recs` is used to generate recommendations. To do that you need to provide the bot with either your profile id on fanfiction.net"
                            " or custom url to your page that you can get [here](https://www.fanfiction.net/account/settings.php)."
                            "\n\nFor the command to work properly, your must have favourited some fics before using it."
                            "Remember that it takes a bit of time for fanfiction.net to update your page so make sure your favourites are displaying before calling {0}recs"
                            "\n\nExamples:"
                            "\n`{0}recs 111` calling command with profile ID"
                            "\n`{0}recs https://www.fanfiction.net/name` calling command with profile url";
    fieldText=fmt::format(fieldText, serverPrefix);
    recsField.value = fieldText;

    SleepyDiscord::EmbedField refreshField;
    refreshField.isInline = true;
    refreshField.name = "Updating recommedations:";
    fieldText = "If you added new favourites to your fanfiction.net page you will need to tell the bot to re-read your page instead of continue using cached version."
                "To do that, you need to repeat yhe command you used to create recommendations with `>refresh` parameter."
                "\n\nExample:\n`{0}recs >refresh 111`"
                "\n\nNote that to prevent bottlenecks while parsing lists from multiple people >refresh option has limited usages per day.";
    fieldText=fmt::format(fieldText, serverPrefix);
    refreshField.value = fieldText;


    SleepyDiscord::EmbedField explanationField;
    explanationField.isInline = false;
    explanationField.name = "How it all works:";
    fieldText = "Recommendations are created by merging a lot of favourite lists of other people on fanfiction.net with tastes similar to yours."
                " Every fic picked from such a list receives a score relative to how close that favourite list is to your tastes."
                " Fic's `Score` value is an accumulated total for each of the favourite lists it appeared on.";
    fieldText=fmt::format(fieldText, serverPrefix);
    explanationField.value = fieldText;

    embed.fields.push_back(recsField);
    embed.fields.push_back(refreshField);
    embed.fields.push_back(explanationField);
//    embed.fields.push_back(exactField);
    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetFreshHelpPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;

    SleepyDiscord::EmbedField freshRecsField;
    freshRecsField.isInline = false;
    freshRecsField.name = "Fresh recommendations mode usage:";
    std::string fieldText = "`{0}fresh` is used to switch your recommedations to display recent fics that have been favourited by people with the same tates as you."
                            " These fics typically have nowhere near enough traction to appear at the top of recommendations but you might find them just as interesting. "
                            " Ideally you want more than one recommendation from other people for such a fic, which is why >strict option exists."
                            "\n\nExamples:"
                            "\n`{0}fresh`"
                            "\n`{0}fresh >strict`"
                            "\n\nTo switch the bot back to top recommendations display, repeat `{0}fresh` command";
    fieldText=fmt::format(fieldText, serverPrefix);
    freshRecsField.value = fieldText;

    embed.fields.push_back(freshRecsField);

    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetRollHelpPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;

    SleepyDiscord::EmbedField rollRecsField;
    rollRecsField.isInline = false;
    rollRecsField.name = "Rolling random fics from recommendations:";
    std::string fieldText = "`{0}roll` is used to display a set of 3 random fics from generated recommendations."
                            " It's an alternative way of fic discovery that is more lenient to how much score fics have for you to see them."
                            " Rolling can be done within the whole of recommendation list or within certain ranges from the top of it that can be supplied with additional parameter to roll command."
                            " The options you have are:"
                            " `all` the same as when `roll` is used without an argument"
                            " `good` will roll within 15% best scores in the list "
                            " `best` will roll within 5% best scores in the list "
                            "\n\nExamples:"
                            "\n`{0}roll`"
                            "\n`{0}roll all`"
                            "\n`{0}roll good`"
                            "\n`{0}roll best`"
                            "\n\nTo roll a new set of 3 fics either repeat roll command or click on emoji below bot's response."
                            "To switch the bot back to top recommendations display, issue `{0}recs` or {0}page command";
    fieldText=fmt::format(fieldText, serverPrefix);
    rollRecsField.value = fieldText;
    embed.fields.push_back(rollRecsField);

    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetFanficFiltersHelpPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;

    SleepyDiscord::EmbedField rollRecsField;
    rollRecsField.isInline = false;
    rollRecsField.name = "Applying filters to fanfics in displayed results:";
    std::string fieldText = "You have a couple options to control which fics are displayed in your recommendations. "
                            "You can hide incomplete and dead fics, show only fics with specific wordcounts, "
                            "show only fics from authors whose fics you've liked already and exclude individual fics from results forever."
                            "These filters can be combined with each other and are controlled with commands below.";
    fieldText=fmt::format(fieldText, serverPrefix);
    rollRecsField.value = fieldText;

    SleepyDiscord::EmbedField completeField;
    completeField.isInline = true;
    completeField.name = "Show only complete fics:";
    fieldText = "`{0}complete` will exclude all of the incomplete fics from your recommendations. This command has no parameters";
    fieldText=fmt::format(fieldText, serverPrefix);
    completeField.value = fieldText;

    SleepyDiscord::EmbedField deadField;
    deadField.isInline = true;
    deadField.name = "Hide dead fics:";
    fieldText = "`{0}dead` will exclude all dead fics from your recommendations. You can supply additional number of days to tell the bot what you consider a `dead` fic."
                "\n\nExamples:\n`{0}dead`\n`{0}dead 400`";
    fieldText=fmt::format(fieldText, serverPrefix);
    deadField.value = fieldText;


    SleepyDiscord::EmbedField xficField;
    xficField.isInline = false;
    xficField.name = "Hide certain fics:";
    fieldText = "`{0}xfic` allows you to exclude individual fics from further display in your recommendations. It accepts two kinds of arguments:"
                "\nID of the fic(s) in the currently displayed list (NOT id on fanfiction.net)"
                "\n`>all` to permanently ignore all fics on the current page"
                "\n\nExamples:"
                "\n`{0}xfic 1`"
                "\n`{0}xfic 3 5 7`"
                "\n`{0}xfic all`";
    fieldText=fmt::format(fieldText, serverPrefix);
    xficField.value = fieldText;


    SleepyDiscord::EmbedField wordsField;
    wordsField.isInline = true;
    wordsField.name = "Set wordcount limits:";
    fieldText = "`{0}words` command allows you to limit wordcount of shown fics. The options you have are `less`, `more` and `between`. The supplied numbers are in thousands of words so `{0}words less 10` means less than 10000 words."
                "\n\nExamples:"
                "\n`{0}words less 50`"
                "\n`{0}words more 100`"
                "\n`{0}words between 50 100`";
    fieldText=fmt::format(fieldText, serverPrefix);
    wordsField.value = fieldText;

    SleepyDiscord::EmbedField likedField;
    likedField.isInline = true;
    likedField.name = "Show fics from trusted authors:";
    fieldText = "`{0}liked` command switches your recommendations to show only fics from authors whose fics you have already favourited. It has no parameters.";
    fieldText=fmt::format(fieldText, serverPrefix);
    likedField.value = fieldText;


    SleepyDiscord::EmbedField resetField;
    resetField.isInline = false;
    resetField.name = "Resetting filters:";
    fieldText = "To reset any currently enabled filter issue `{0}xfilter` command.";
    fieldText=fmt::format(fieldText, serverPrefix);
    resetField.value = fieldText;


    embed.fields.push_back(rollRecsField);
    embed.fields.push_back(deadField);
    embed.fields.push_back(wordsField);
    embed.fields.push_back(xficField);
    embed.fields.push_back(completeField);
    embed.fields.push_back(likedField);
    embed.fields.push_back(resetField);

    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetFandomFiltersHelpPage(std::string_view serverPrefix)
{
    SleepyDiscord::Embed embed;

    SleepyDiscord::EmbedField descriptionField;
    descriptionField.isInline = false;
    descriptionField.name = "Fandom controls:";
    std::string fieldText = "You can control fandoms of displayed fics in two ways : you can permanently ignore fandoms or set fandom of currently displayed fics. "
                            "These options are controlled with `{0}xfandom` and `{0}fandom` commands respectively.";

    fieldText=fmt::format(fieldText, serverPrefix);
    descriptionField.value = fieldText;

    SleepyDiscord::EmbedField ignoreFandomField;
    ignoreFandomField.isInline = false;
    ignoreFandomField.name = "Ignoring fandoms:";
    fieldText = "`{0}xfandom` allows you to ignore a fandom and exclude it from show recommendations. You also have an option to ignore crossovers with this fandom."
                " Fandom names are case insensitive but need written exactly the same as the bot displays them to you (i.e. without japanese letters and weird symbols."
                "\nRepeating the fandom ignore command resets ignore for this fandom."
                "\nUsing `>reset` as an optional parameter clears your whole fandom ignore list."
                "\n\nExamples:"
                "\n`{0}xfandom harry potter`"
                "\n`{0}xfandom >full harry potter`"
                "\n`{0}xfandom >reset`";


    fieldText=fmt::format(fieldText, serverPrefix);
    ignoreFandomField.value = fieldText;

    SleepyDiscord::EmbedField filterFandomField;
    filterFandomField.isInline = false;
    filterFandomField.name = "Filtering fandoms:";
    fieldText = "You can display only fics from specific fandom with `{0}fandom` command. "
                "\nRepeating `{0}fandom` with another fandom will display exact crossovers."
                "Using `>reset` instead of fandom name will reset the fandom filter."
                "\n\nExamples:"
                "\n`{0}fandom harry potter`"
                "\n`{0}fandom fairy tail`"
                "\n`{0}fandom >reset`";

    fieldText=fmt::format(fieldText, serverPrefix);
    filterFandomField.value = fieldText;


    embed.fields.push_back(descriptionField);
    embed.fields.push_back(ignoreFandomField);
    embed.fields.push_back(filterFandomField);

    embed.color = 0xff0000;
    return embed;
}


}

