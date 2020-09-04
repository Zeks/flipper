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
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::WriteUser(db, user);
}

int Users::WriteUserList(QString user_id, QString list_name, discord::EListType list_type, int min_match, int match_ratio, int always_at)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    return database::discord_quries::WriteUserList(db, user_id, list_name, list_type, min_match, match_ratio, always_at).data;
}

bool Users::DeleteUserList(QString user_id, QString list_name)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    return database::discord_quries::DeleteUserList(db, user_id, list_name).data;
}

void Users::IgnoreFandom(QString userId, int fandomId, bool ignoreCrosses)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::IgnoreFandom(db, userId, fandomId, ignoreCrosses);
}

void Users::UnignoreFandom(QString userId, int fandomId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::UnignoreFandom(db, userId, fandomId);
}

discord::FandomFilter Users::GetIgnoreList(QString userId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    return database::discord_quries::GetFandomIgnoreList(db, userId).data;
}

void Users::TagFanfic(QString userId, QString tag, int ficId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::TagFanfic(db, userId, ficId, tag);
}

void Users::UnTagFanfic(QString userId, QString tag, int ficId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::TagFanfic(db, userId, ficId, tag);
}

void Users::BanUser(QString userId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::BanUser(db, userId);
}

void Users::UpdateCurrentPage(QString userId, int page)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::UpdateCurrentPage(db, userId, page);
}

void Users::UnfilterFandom(QString userId, int fandomId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::UnfilterFandom(db, userId, fandomId);
}

void Users::ResetFandomFilter(QString userId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::ResetFandomFilter(db, userId);
}

void Users::ResetFandomIgnores(QString userId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::ResetFandomIgnores(db, userId);
}

void Users::ResetFicIgnores(QString userId)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::ResetFicIgnores(db, userId);
}

void Users::FilterFandom(QString userId, int fandomId, bool allowCrossovers)
{
    std::lock_guard<std::mutex> guard(dbMutex);
    database::discord_quries::FilterFandom(db, userId, fandomId, allowCrossovers);
}



}
