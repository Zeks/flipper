/*
FFSSE is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "Interfaces/ffn/ffn_fanfics.h"
#include "pure_sql.h"

namespace interfaces {



int FFNFanfics::GetIDFromWebID(int value)
{
    return Fanfics::GetIDFromWebID(value, "ffn");
}

int FFNFanfics::GetWebIDFromID(int value)
{
    return Fanfics::GetWebIDFromID(value, "ffn");
}

bool FFNFanfics::DeactivateFic(int ficId)
{
    return database::puresql::DeactivateStory(ficId, "ffn", db);
}

int FFNFanfics::GetIdForUrl(QString url)
{
    return -1; //! todo
}

bool FFNFanfics::IsDataLoaded()
{
    return true;
}

bool FFNFanfics::Sync(bool forcedSync)
{
    return true;
}

bool FFNFanfics::Load()
{
    return true;
}

void FFNFanfics::Clear()
{

}

}
