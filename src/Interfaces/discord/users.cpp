/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include "Interfaces/discord/users.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/db_interface.h"
#include "sql/discord/discord_queries.h"
#include "discord/discord_user.h"
#include "discord/db_vendor.h"
#include "include/pure_sql.h"
#include <QSqlQuery>
#include <QDebug>

namespace interfaces {

Users::~Users()
{

}

QSharedPointer<discord::User> Users::GetUser(QString user)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    return database::discord_queries::GetUser(dbToken->db, user).data;
}

void Users::WriteUser(QSharedPointer<discord::User> user)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::WriteUser(dbToken->db, user);
}

void Users::WriteUserFFNId(QString user_id, int ffn_id)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::WriteUserFFNId(dbToken->db, user_id, ffn_id);
}

int Users::WriteUserList(QString user_id, QString list_name, discord::EListType list_type, int min_match, int match_ratio, int always_at)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    return database::discord_queries::WriteUserList(dbToken->db, user_id, list_name, list_type, min_match, match_ratio, always_at).data;
}

bool Users::WriteForcedListParams(QString user_id, int forceMinMatches, int forcedRatio)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    return database::discord_queries::WriteForcedListParams(dbToken->db, user_id, forceMinMatches, forcedRatio).data;
}

bool Users::DeleteUserList(QString user_id, QString list_name)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    return database::discord_queries::DeleteUserList(dbToken->db, user_id, list_name).data;
}

void Users::IgnoreFandom(QString userId, int fandomId, bool ignoreCrosses)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::IgnoreFandom(dbToken->db, userId, fandomId, ignoreCrosses);
}

void Users::UnignoreFandom(QString userId, int fandomId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::UnignoreFandom(dbToken->db, userId, fandomId);
}

discord::FandomFilter Users::GetIgnoreList(QString userId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    return database::discord_queries::GetFandomIgnoreList(dbToken->db, userId).data;
}

void Users::TagFanfic(QString userId, QString tag, int ficId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::TagFanfic(dbToken->db, userId, ficId, tag);
}

void Users::UnTagFanfic(QString userId, QString tag, int ficId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::TagFanfic(dbToken->db, userId, ficId, tag);
}

void Users::BanUser(QString userId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::BanUser(dbToken->db, userId);
}

void Users::UpdateCurrentPage(QString userId, int page)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::UpdateCurrentPage(dbToken->db, userId, page);
}

void Users::UnfilterFandom(QString userId, int fandomId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::UnfilterFandom(dbToken->db, userId, fandomId);
}

void Users::ResetFandomFilter(QString userId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::ResetFandomFilter(dbToken->db, userId);
}

void Users::ResetFandomIgnores(QString userId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::ResetFandomIgnores(dbToken->db, userId);
}

void Users::ResetFicIgnores(QString userId)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::ResetFicIgnores(dbToken->db, userId);
}

void Users::FilterFandom(QString userId, int fandomId, bool allowCrossovers)
{
    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
    database::discord_queries::FilterFandom(dbToken->db, userId, fandomId, allowCrossovers);
}



}
