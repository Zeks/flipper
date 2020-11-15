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

void FfnPages::AddPage(QSharedPointer<discord::FFNPage> page)
{
    QWriteLocker locker(&lock);
    pages[page->getId()] = page;
}

bool FfnPages::HasPage(const std::string & id)
{
    QReadLocker locker(&lock);
    if(pages.contains(id))
        return true;
    return false;
}

void FfnPages::UpdatePageFromAction(const std::string & pageId, int favourites)
{

    bool pageLoaded = HasPage(pageId);
    QSharedPointer<FFNPage> page;

    if(pageLoaded)
        page = GetPage(pageId);
    else if(LoadPage(pageId)){
        page = GetPage(pageId);
    }
    else{
        // page isn't in the database, need to create it there first
        page.reset(new FFNPage);
        page->setId(pageId);
        page->setFavourites(favourites);
        page->setLastParsed(QDate::currentDate());
        page->setTotalParseCounter(0);
        page->setDailyParseCounter(0);
        {
            auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
            database::discord_queries::WriteFFNPage(dbToken->db, page);
        }
        {
            QWriteLocker locker(&lock);
            pages[pageId] = page;
        }
    }
    if(page){
        page->setFavourites(favourites);
        page->setTotalParseCounter(page->getTotalParseCounter()+1);
        if(page->getLastParsed() != QDate::currentDate())
            page->setDailyParseCounter(1);
        else
            page->setDailyParseCounter(page->getDailyParseCounter()+1);
        page->setLastParsed(QDate::currentDate());
        {
            auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
            database::discord_queries::UpdateFFNPage(dbToken->db, page);
        }
    }
}

QSharedPointer<discord::FFNPage> FfnPages::GetPage(const std::string & id) const
{
    QReadLocker locker(&lock);
    if(!pages.contains(id))
        return {};
    return pages[id];
}

bool FfnPages::LoadPage(const std::string & id)
{
    QWriteLocker locker(&lock);
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
    auto page = database::discord_queries::GetFFNPage(dbToken->db, id).data;
    if(!page)
        return false;

    pages[id] = page;
    return true;
}
}
