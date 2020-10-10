#include "discord/help_generator.h"
#include "discord/type_strings.h"
#include "fmt/format.h"

namespace discord {

SleepyDiscord::Embed GetHelpPage(int pageNumber, std::string serverPrefix)
{
    switch(pageNumber){
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

}
