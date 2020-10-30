#pragma once

#include <functional>
#include <QString>
#include <memory>
#include <array>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include "core/section.h"
#include "regex_utils.h"
#include "transaction.h"
#include "Interfaces/discord/users.h"
#include "discord/discord_user.h"
#include "discord/discord_server.h"
#include "discord/fandom_filter_token.h"
#include "sqlcontext.h"
namespace database {
using namespace puresql;
namespace discord_queries{

    DiagnosticSQLResult<QSharedPointer<discord::User>> GetUser(QSqlDatabase db, QString);
    DiagnosticSQLResult<QSharedPointer<discord::Server>> GetServer(QSqlDatabase db, const std::string& server_id);
    DiagnosticSQLResult<discord::FandomFilter> GetFandomIgnoreList(QSqlDatabase db, QString userId);
    DiagnosticSQLResult<discord::FandomFilter> GetFilterList(QSqlDatabase db, QString userId);
    DiagnosticSQLResult<QSet<int>> GetFicIgnoreList(QSqlDatabase db, QString userId);
    DiagnosticSQLResult<int> GetCurrentPage(QSqlDatabase db, QString userId);

    DiagnosticSQLResult<bool> WriteUser(QSqlDatabase db, QSharedPointer<discord::User>);
    DiagnosticSQLResult<bool> WriteServer(QSqlDatabase db, QSharedPointer<discord::Server>);
    DiagnosticSQLResult<bool> WriteServerPrefix(QSqlDatabase db, const std::string&, QString);
    DiagnosticSQLResult<bool> WriteUserFFNId(QSqlDatabase db, QString user_id, int ffn_id);
    DiagnosticSQLResult<bool> DeleteUserList(QSqlDatabase db, QString user_id, QString list_name);
    DiagnosticSQLResult<int> WriteUserList(QSqlDatabase db, QString user_id,
                                           QString list_name, discord::EListType list_type,
                                           int min_match, int match_ratio, int always_at);
    DiagnosticSQLResult<bool> WriteForcedListParams(QSqlDatabase db, QString user_id, int forceMinMatches, int forcedRatio);
    DiagnosticSQLResult<bool> WriteForceLikedAuthors(QSqlDatabase db, QString , bool);
    DiagnosticSQLResult<bool> WriteFreshSortingParams(QSqlDatabase db, QString user_id, bool, bool);
    DiagnosticSQLResult<bool> SetHideDeadFilter(QSqlDatabase db, QString , bool);
    DiagnosticSQLResult<bool> SetCompleteFilter(QSqlDatabase db, QString , bool);


    DiagnosticSQLResult<bool> IgnoreFandom(QSqlDatabase db, QString user_id, int fandom_id, bool including_crossovers);
    DiagnosticSQLResult<bool> UnignoreFandom(QSqlDatabase db, QString userId, int fandomId);
    DiagnosticSQLResult<bool> TagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag);
    DiagnosticSQLResult<bool> UnTagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag); // empty tag removes all
    DiagnosticSQLResult<bool> BanUser(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> UnbanUser(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> UpdateCurrentPage(QSqlDatabase db, QString user_id, int page);
    DiagnosticSQLResult<bool> UnfilterFandom(QSqlDatabase db, QString user_id, int fandomId);
    DiagnosticSQLResult<bool> ResetFandomFilter(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> ResetFandomIgnores(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> ResetFicIgnores(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> FillUserUids(QSqlDatabase db);
    DiagnosticSQLResult<bool> FilterFandom(QSqlDatabase db, QString user_id, int fandom_id, bool allow_crossovers);
    DiagnosticSQLResult<bool> CompletelyRemoveUser(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> SetWordcountFilter(QSqlDatabase db, QString userId, discord::WordcountFilter);
    DiagnosticSQLResult<bool> SetDeadFicDaysRange(QSqlDatabase db, QString userId, int days);

 }
}