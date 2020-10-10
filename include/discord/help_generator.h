#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

namespace discord {
    SleepyDiscord::Embed GetHelpPage(int pageNumber, std::string serverPrefix);
    SleepyDiscord::Embed  GetTopLevelHelpPage(std::string serverPrefix);
}
