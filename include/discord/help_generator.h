#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

namespace discord {
enum class EHelpPages{
    general_help = 0,
    list_navigation_help = 1,
    list_modes_help = 2,
    recs_help = 3,
    fresh_help = 4,
    roll_help = 5,
    fanfic_filter_help = 6,
    fandom_filter_help = 7,
    last_help_page = 7,
};

    SleepyDiscord::Embed GetHelpPage(int pageNumber, std::string_view serverPrefix);
    SleepyDiscord::Embed  GetTopLevelHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetListNavigationPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetListModesPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetRecsHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetFreshHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetRollHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetFanficFiltersHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetFandomFiltersHelpPage(std::string_view serverPrefix);

    int GetHelpPage(std::string_view argument);
}
