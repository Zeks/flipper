#include "discord/help_generator.h"
#include "discord/type_strings.h"
#include "fmt/format.h"

namespace discord {

SleepyDiscord::Embed GetHelpPage(int pageNumber, std::string serverPrefix)
{
    switch(pageNumber){
    case 0:
        return GetTopLevelHelpPage(serverPrefix);
    case 1:
        return GetListNavigationPage(serverPrefix);
    case 2:
        return GetListModesPage(serverPrefix);
    case 3:
        return GetRecsHelpPage(serverPrefix);

    default:
        return GetTopLevelHelpPage(serverPrefix);
    }
}

SleepyDiscord::Embed GetTopLevelHelpPage(std::string serverPrefix)
{
    SleepyDiscord::Embed embed;
    std::string header = "To start using the bot you will need to do `{0}recs X` where X is your profile id on fanfiction.net or url to that.\n"
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
    listTypesText += std::string(TypeStringHolder<SimilarFicsCommand>::shorthand) + "\n";
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
    embed.fields.push_back(supportField);

    SleepyDiscord::EmbedFooter footer;
    std::string footerText = "To reset all your data in the bot, issue `{0}purge`";
    footerText=fmt::format(footerText, serverPrefix);
    footer.text = footerText;
    embed.footer = footer;
    embed.color = 0xff0000;
    return embed;
}

SleepyDiscord::Embed GetListNavigationPage(std::string serverPrefix)
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

SleepyDiscord::Embed GetListModesPage(std::string serverPrefix)
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

SleepyDiscord::Embed GetRecsHelpPage(std::string serverPrefix)
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


}

