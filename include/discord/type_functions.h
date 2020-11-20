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
#include "discord/type_strings.h"
#include "third_party/str_concat.h"
#include "third_party/ctre.hpp"
#include <QSharedPointer>

namespace str {
    static constexpr std::string_view separator = ")|(";
    static constexpr std::string_view leftBrace= "(";
    static constexpr std::string_view rightBrace = ")";
    static constexpr std::string_view stringStart = "^(";
}

namespace discord {
    constexpr std::string_view GetSimplePatternChecker()
    {
        return join_v<str::stringStart, str::leftBrace,TypeStringHolder<RecsCreationCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<NextPageCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<PreviousPageCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<PageChangeCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<SetFandomCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<IgnoreFandomCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<IgnoreFicCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<DisplayHelpCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<RngCommand>::prefixlessPattern,str::separator,
                TypeStringHolder<FilterLikedAuthorsCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<WordcountCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ShowFreshRecsCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ShowCompletedCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<HideDeadCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<SendMessageToChannelCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ToggleBanCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<StatsCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<GemsCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ChangeTargetCommand>::prefixlessPattern, str::separator,
                //TypeStringHolder<ShowFullFavouritesCommand>::prefixlessPattern, str::separator,
                //TypeStringHolder<SimilarFicsCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ChangeServerPrefixCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ChangePermittedChannelCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<PurgeCommand>::prefixlessPattern, str::separator,
                TypeStringHolder<ResetFiltersCommand>::prefixlessPattern, str::rightBrace, str::rightBrace>;
    }

template<typename T>
constexpr auto matchCommand(std::string_view sv) noexcept {
    return ctre::search<TypeStringHolder<T>::pattern>(sv);
}

}

