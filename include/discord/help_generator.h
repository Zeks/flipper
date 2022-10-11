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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wunused-parameter"
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
    gems_help = 5,
    roll_help = 6,
    fanfic_filter_help = 7,
    fandom_filter_help = 8,
    last_help_page = 8,
};

    SleepyDiscord::Embed GetHelpPage(int pageNumber, std::string_view serverPrefix);
    SleepyDiscord::Embed  GetTopLevelHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetListNavigationPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetListModesPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetRecsHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetFreshHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetGemsHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetRollHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetFanficFiltersHelpPage(std::string_view serverPrefix);
    SleepyDiscord::Embed  GetFandomFiltersHelpPage(std::string_view serverPrefix);

    int GetHelpPage(std::string_view argument);
}
