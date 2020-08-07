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
#include "include/pure_sql.h"
#include <QSqlQuery>
#include <QDebug>

namespace interfaces {

Users::~Users()
{

}

QSharedPointer<discord::User> Users::GetUser(QString user)
{
    return database::discord_quries::GetUser(db, user).data;
}

void Users::WriteUser(QSharedPointer<discord::User> user)
{
    database::discord_quries::WriteUser(db, user);
}

int Users::WriteUserList(QString user_id, QString list_name, discord::EListType list_type, int min_match, int match_ratio, int always_at)
{
    return database::discord_quries::WriteUserList(db, user_id, list_name, list_type, min_match, match_ratio, always_at).data;
}

void Users::IgnoreFandom(QString userId, int fandomId, bool ignoreCrosses)
{
    database::discord_quries::IgnoreFandom(db, userId, fandomId, ignoreCrosses);
}

void Users::UnignoreFandom(QString userId, int fandomId)
{
    database::discord_quries::UnignoreFandom(db, userId, fandomId);
}

QList<discord::IgnoreFandom> Users::GetIgnoreList(QString userId)
{
    return database::discord_quries::GetIgnoreList(db, userId).data;
}

void Users::TagFanfic(QString userId, QString tag, int ficId)
{
    database::discord_quries::TagFanfic(db, userId, ficId, tag);
}

void Users::UnTagFanfic(QString userId, QString tag, int ficId)
{
    database::discord_quries::TagFanfic(db, userId, ficId, tag);
}

void Users::BanUser(QString userId)
{
    database::discord_quries::BanUser(db, userId);
}

void Users::UpdateCurrentPage(QString userId, int page)
{
    database::discord_quries::UpdateCurrentPage(db, userId, page);
}



}
