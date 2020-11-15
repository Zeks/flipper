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
#include "discord/discord_ffn_page.h"
#include "discord/discord_init.h"
#include "discord/db_vendor.h"
#include "sql/discord/discord_queries.h"

namespace discord{

int FFNPage::getTotalParseCounter() const
{
    return totalParseCounter;
}

void FFNPage::setTotalParseCounter(int value)
{
    QWriteLocker locker(&lock);
    totalParseCounter = value;
}

int FFNPage::getDailyParseCounter() const
{
    return dailyParseCounter;
}

void FFNPage::setDailyParseCounter(int value)
{
    QWriteLocker locker(&lock);
    dailyParseCounter = value;
}

int FFNPage::getFavourites() const
{
    return favourites;
}

void FFNPage::setFavourites(int value)
{
    QWriteLocker locker(&lock);
    favourites = value;
}

QDate FFNPage::getLastParsed() const
{
    return lastParsed;
}

void FFNPage::setLastParsed(const QDate &value)
{
    QWriteLocker locker(&lock);
    lastParsed = value;
}

std::string FFNPage::getId() const
{
    return id;
}

void FFNPage::setId(const std::string &value)
{
    QWriteLocker locker(&lock);
    id = value;
}

void discord::FfnPages::AddPage(QSharedPointer<discord::FFNPage> page)
{
    QWriteLocker locker(&lock);
    pages[page->getId()] = page;
}

bool discord::FfnPages::HasPage(const std::string & id)
{
    QReadLocker locker(&lock);
    if(pages.contains(id))
        return true;
    return false;
}

QSharedPointer<discord::FFNPage> discord::FfnPages::GetPage(const std::string & id) const
{
    if(!pages.contains(id))
        return {};
    return pages[id];
}

bool discord::FfnPages::LoadPage(const std::string & id)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    auto page = database::discord_queries::GetFFNPage(dbToken->db, id).data;
    if(!page)
        return false;

    pages[id] = page;
    return true;
}
}
