/*
Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#pragma once

#include <functional>
#include <QString>
#include <memory>
#include <array>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_query.h"
#include <QSqlError>
#include <QSharedPointer>
#include "core/section.h"
#include "regex_utils.h"
#include "transaction.h"
#include "Interfaces/discord/users.h"
#include "discord/discord_user.h"
#include "discord/discord_ffn_page.h"
#include "discord/discord_server.h"
#include "discord/fandom_filter_token.h"
#include "discord/review.h"
#include "sql_abstractions/sql_context.h"
namespace database {
using namespace sql;
namespace discord_queries{

    DiagnosticSQLResult<QSharedPointer<discord::User>> GetUser(sql::Database db, QString);
    DiagnosticSQLResult<QSharedPointer<discord::Server>> GetServer(sql::Database db, const std::string& server_id);
    DiagnosticSQLResult<QSharedPointer<discord::FFNPage>> GetFFNPage(sql::Database db, const std::string& pageId);

    DiagnosticSQLResult<discord::FandomFilter> GetFandomIgnoreList(sql::Database db, QString userId);
    DiagnosticSQLResult<discord::FandomFilter> GetFilterList(sql::Database db, QString userId);
    DiagnosticSQLResult<QSet<int>> GetFicIgnoreList(sql::Database db, QString userId);
    DiagnosticSQLResult<int> GetCurrentPage(sql::Database db, QString userId);

    DiagnosticSQLResult<bool> WriteUser(sql::Database db, QSharedPointer<discord::User>);
    DiagnosticSQLResult<bool> WriteServer(sql::Database db, QSharedPointer<discord::Server>);
    DiagnosticSQLResult<bool> WriteFFNPage(sql::Database db, QSharedPointer<discord::FFNPage>);
    DiagnosticSQLResult<bool> UpdateFFNPage(sql::Database db, QSharedPointer<discord::FFNPage>);
    DiagnosticSQLResult<bool> WriteServerPrefix(sql::Database db, const std::string&, QString);
    DiagnosticSQLResult<bool> WriteServerDedicatedChannel(sql::Database db, const std::string&, const std::string&);
    DiagnosticSQLResult<bool> WriteUserFFNId(sql::Database db, QString user_id, int ffn_id);
    DiagnosticSQLResult<bool> WriteUserFavouritesSize(sql::Database db, QString, int);
    DiagnosticSQLResult<bool> DeleteUserList(sql::Database db, QString user_id, QString list_name);
    DiagnosticSQLResult<int> WriteUserList(sql::Database db, QString user_id,
                                           QString list_name, discord::EListType list_type,
                                           int min_match, int match_ratio, int always_at);
    DiagnosticSQLResult<bool> WriteForcedListParams(sql::Database db, QString user_id, int forceMinMatches, int forcedRatio);
    DiagnosticSQLResult<bool> WriteForceLikedAuthors(sql::Database db, QString , bool);
    DiagnosticSQLResult<bool> WriteFreshSortingParams(sql::Database db, QString user_id, bool, bool);
    DiagnosticSQLResult<bool> WriteGemSortingParams(sql::Database db, QString user_id, bool);

    DiagnosticSQLResult<bool> WriteLargeListReparseToken(sql::Database db, QString user_id, discord::LargeListToken);
    DiagnosticSQLResult<bool> UpdateUsername(sql::Database db, QString , QString);
    DiagnosticSQLResult<bool> SetHideDeadFilter(sql::Database db, QString , bool);
    DiagnosticSQLResult<bool> SetCompleteFilter(sql::Database db, QString , bool);

    DiagnosticSQLResult<bool> IgnoreFandom(sql::Database db, QString user_id, int fandom_id, bool including_crossovers);
    DiagnosticSQLResult<bool> UnignoreFandom(sql::Database db, QString userId, int fandomId);
    DiagnosticSQLResult<bool> TagFanfic(sql::Database db, QString user_id, int fic_id, QString fic_tag);
    DiagnosticSQLResult<bool> UnTagFanfic(sql::Database db, QString user_id, int fic_id, QString fic_tag); // empty tag removes all
    DiagnosticSQLResult<bool> BanUser(sql::Database db, QString user_id);
    DiagnosticSQLResult<bool> UnbanUser(sql::Database db, QString user_id);
    DiagnosticSQLResult<bool> BanServer(sql::Database db, QString server_id);
    DiagnosticSQLResult<bool> UnbanServer(sql::Database db, QString server_id);

    DiagnosticSQLResult<bool> UpdateCurrentPage(sql::Database db, QString user_id, int page);
    DiagnosticSQLResult<bool> UnfilterFandom(sql::Database db, QString user_id, int fandomId);
    DiagnosticSQLResult<bool> ResetFandomFilter(sql::Database db, QString user_id);
    DiagnosticSQLResult<bool> ResetFandomIgnores(sql::Database db, QString user_id);
    DiagnosticSQLResult<bool> ResetFicIgnores(sql::Database db, QString user_id);
    DiagnosticSQLResult<bool> FillUserUids(sql::Database db);
    DiagnosticSQLResult<bool> FilterFandom(sql::Database db, QString user_id, int fandom_id, bool allow_crossovers);
    DiagnosticSQLResult<bool> CompletelyRemoveUser(sql::Database db, QString user_id);
    DiagnosticSQLResult<bool> SetWordcountFilter(sql::Database db, QString userId, discord::WordcountFilter);
    DiagnosticSQLResult<bool> SetRecommendationsCutoff(sql::Database db, QString userId, int);

    DiagnosticSQLResult<bool> SetDeadFicDaysRange(sql::Database db, QString userId, int days);
    DiagnosticSQLResult<bool> SetDateFilter(sql::Database db, QString userId, filters::EDateFilterType, QString);

    DiagnosticSQLResult<bool> RemoveReview(sql::Database db, QString reviewId);
    DiagnosticSQLResult<QString> GetReviewAuthor(sql::Database db, QString reviewId);
    DiagnosticSQLResult<std::vector<std::string>> GetReviewList(sql::Database db, discord::ReviewFilter);
    //DiagnosticSQLResult<bool> AddReview(sql::Database db, QString user_id, QString server_id, QString raw_url, QString site_type, QString site_identifier,int score, QString review,QString review_id);
    DiagnosticSQLResult<bool> AddReview(sql::Database db, const discord::FicReview&);

    DiagnosticSQLResult<discord::FicReview> GetReview(sql::Database db, std::string);



 }
}
