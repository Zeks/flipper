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
#include "sql/discord/discord_queries.h"
#include "logger/QsLog.h"

template <typename T>
bool NullPtrGuard(T item)
{
    if(!item)
    {
        qDebug() << "attempted to fill a nullptr";
        return false;
    }
    return true;
}
#define DATAQ [&](auto& data, auto q)
#define DATAQ1 [&](auto& , auto q)
#define COMMAND(NAME)  { #NAME, NAME}

#define BP1(A) {COMMAND(A)}
#define BP2(A, B) {COMMAND(A), COMMAND(B)}
#define BP3(A, B, C) {COMMAND(A), COMMAND(B), COMMAND(C)}
#define BP4(A, B, C, D) {COMMAND(A), COMMAND(B), COMMAND(C), COMMAND(D)}
#define BP8(A, B, C, D, E, F, G, H) {COMMAND(A), COMMAND(B), COMMAND(C),COMMAND(D), COMMAND(E), COMMAND(F), COMMAND(G), COMMAND(H)}
//#define BP3(X, Y, Z) {{"X", X}, {"Y", Y},{"Z", Z}}

namespace database {
namespace discord_queries{



DiagnosticSQLResult<QSharedPointer<discord::User>> GetUser(QSqlDatabase db, QString user_id){
    QString qs = QString("select * from discord_users where user_id = :user_id");

    SqlContext<QSharedPointer<discord::User>> ctx(db, qs, BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<discord::User> user(new discord::User);
        user->SetUserID(user_id);
        user->SetUserName(q.value("user_name").toString());
        user->SetFfnID(q.value("ffn_id").toString());
        user->SetCurrentListId(q.value("current_list").toInt());
        user->SetBanned(q.value("banned").toBool());
        user->SetUseLikedAuthorsOnly(q.value("use_liked_authors_only").toBool());
        user->SetUuid(q.value("uuid").toString());
        user->SetForcedMinMatch(q.value("forced_min_matches").toInt());
        user->SetForcedRatio(q.value("forced_ratio").toInt());
        ctx.result.data = user;
    });
    return ctx.result;
}

DiagnosticSQLResult<QSharedPointer<discord::Server>> GetServer(QSqlDatabase db, const std::string& serverId){
    QString qs = QString("select * from discord_servers where server_id = :server_id");

    QString server_id = QString::fromStdString(serverId);
    SqlContext<QSharedPointer<discord::Server>> ctx(db, qs, BP1(server_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<discord::Server> server(new discord::Server);
        server->SetServerId(serverId);

        server->SetBanned(q.value("server_banned").toBool());
        server->SetSilenced(q.value("server_silenced").toBool());
        server->SetAnswerInPm(q.value("bot_answers_in_pm").toBool());

        server->SetParserRequestLimit(q.value("parse_request_limit").toInt());
        server->SetTotalRequests(q.value("total_requests").toInt());

        server->SetFirstActive(q.value("active_since").toDateTime());
        server->SetLastActive(q.value("last_request").toDateTime());


        server->SetOwnerId(q.value("owner_id").toString());
        server->SetCommandPrefix(q.value("command_prefix").toString());
        server->SetServerName(q.value("server_name").toString());

        ctx.result.data = server;
    });
    return ctx.result;
}



DiagnosticSQLResult<discord::FandomFilter> GetFandomIgnoreList(QSqlDatabase db, QString user_id){
    QString qs = QString("select * from ignored_fandoms where user_id = :user_id");

    SqlContext<discord::FandomFilter> ctx(db, qs, BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.AddFandom(q.value("fandom_id").toInt(),
                                  q.value("including_crossovers").toBool());
    });
    return ctx.result;
}

DiagnosticSQLResult<discord::FandomFilter> GetFilterList(QSqlDatabase db, QString user_id){
    QString qs = QString("select * from filtered_fandoms where user_id = :user_id");

    SqlContext<discord::FandomFilter> ctx(db, qs, BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.AddFandom(q.value("fandom_id").toInt(),
                                  q.value("including_crossovers").toBool());
    });
    return ctx.result;
}

DiagnosticSQLResult<QSet<int>> GetFicIgnoreList(QSqlDatabase db, QString user_id)
{
    QString qs = QString("select fic_id from fic_tags where user_id = :user_id and fic_tag = 'ignored'");

    SqlContext<QSet<int>> ctx(db, qs, BP1(user_id));
    ctx.FetchLargeSelectIntoList<int>("fic_id",qs);
    return ctx.result;
}

DiagnosticSQLResult<int> GetCurrentPage(QSqlDatabase db, QString user_id)
{
    QString qs = QString("select at_page from user_lists where user_id = :user_id");

    SqlContext<int> ctx(db, qs, BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = q.value("at_page").toInt();
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteUser(QSqlDatabase db, QSharedPointer<discord::User> user){
    QString qs = "INSERT INTO discord_users(user_id, user_name, ffn_id, reads_slash, current_list, banned) values(:user_id, :user_name, :ffn_id, :reads_slash, 0, 0)";
    SqlContext<bool> ctx(db, qs);
    ctx.bindValue("user_id", user->UserID());
    ctx.bindValue("user_name", user->UserName());
    ctx.bindValue("ffn_id", user->FfnID());
    ctx.bindValue("reads_slash", user->ReadsSlash());
    ctx.ExecAndCheck(false);
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteServer(QSqlDatabase db, QSharedPointer<discord::Server> server)
{
    QString qs = "INSERT INTO discord_servers(server_id, server_name, active_since, last_request) "
                 "values(:server_id, :server_name, :active_since, :last_request)";
    SqlContext<bool> ctx(db, qs);
    ctx.bindValue("server_id", QString::fromStdString(server->GetServerId()));
    ctx.bindValue("server_name", server->GetServerName());
    ctx.bindValue("active_since", server->GetFirstActive());
    ctx.bindValue("last_request", server->GetLastActive());
    ctx.ExecAndCheck(false);
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteServerPrefix(QSqlDatabase db, const std::string& serverId, QString new_prefix)
{
    QString qs = "update discord_servers set command_prefix = :new_prefix where server_id = :server_id";
    QString server_id = QString::fromStdString(serverId);
    SqlContext<bool> ctx(db, qs, BP2(server_id, new_prefix));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<int> WriteUserList(QSqlDatabase db, QString user_id, QString list_name,
                                       discord::EListType list_type,
                                       int min_match, int match_ratio, int always_at)
{
    QString qs = "INSERT INTO user_lists(user_id, list_name, list_type, min_match, match_ratio, always_at,  list_id, generated)"
                 " values(:user_id, :list_name, :list_type, :min_match, :match_ratio, :always_at, :list_id, :generated)";
    int list_id = 0;
    auto generated= QDateTime::currentDateTimeUtc();
    SqlContext<bool> ctx(db, qs, BP8(user_id, list_name, list_type, min_match, match_ratio, always_at, list_id, generated));
    ctx.ExecAndCheck(false);
    DiagnosticSQLResult<int> result;
    result.data = 0;
    result.success = ctx.result.success;
    result.oracleError = ctx.result.oracleError;
    return result;
}

DiagnosticSQLResult<bool> DeleteUserList(QSqlDatabase db, QString user_id, QString list_name)
{
    QString qs = "delete from user_lists where user_id = :user_id and list_id = 0";
    auto generated= QDateTime::currentDateTimeUtc();
    SqlContext<bool> ctx(db, qs, BP2(user_id, list_name));
    ctx.ExecAndCheck(false);
    return ctx.result;
}


DiagnosticSQLResult<bool> IgnoreFandom(QSqlDatabase db, QString user_id, int fandom_id, bool including_crossovers){
    UnignoreFandom(db, user_id, fandom_id);
    QString qs = "INSERT INTO ignored_fandoms(user_id, fandom_id,including_crossovers) values(:user_id, :fandom_id, :including_crossovers)";
    SqlContext<bool> ctx(db, qs, BP3(user_id, fandom_id, including_crossovers));
    ctx.ExecAndCheck(false);
    return ctx.result;
}
DiagnosticSQLResult<bool> UnignoreFandom(QSqlDatabase db, QString user_id, int fandom_id){
    QString qs = "delete from ignored_fandoms where user_id= :user_id and fandom_id = :fandom_id";
    SqlContext<bool> ctx(db, qs, BP2(user_id, fandom_id));
    ctx.ExecAndCheck(false);
    return ctx.result;
}
DiagnosticSQLResult<bool> TagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag){
    QString qs = "INSERT INTO fic_tags(user_id, fic_id,fic_tag) values(:user_id, :fic_id, :fic_tag)";
    SqlContext<bool> ctx(db, qs, BP3(user_id, fic_id, fic_tag));
    ctx.ExecAndCheck(false);
    return ctx.result;
}
DiagnosticSQLResult<bool> UnTagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag){
    if(fic_tag.isEmpty())
    {
        QString qs = "delete from fic_tags where user_id= :user_id and fic_id = :fic_id";
        SqlContext<bool> ctx(db, qs, BP2(user_id, fic_id));
        ctx.ExecAndCheck(false);
        return ctx.result;
    }
    else {
        QString qs = "delete from fic_tags where user_id= :user_id and fic_id = :fic_id and fic_tag = :fic_tag";
        SqlContext<bool> ctx(db, qs, BP3(user_id, fic_id, fic_tag));
        ctx.ExecAndCheck(false);
        return ctx.result;
    }

}
DiagnosticSQLResult<bool> BanUser(QSqlDatabase db, QString user_id){
    QString qs = "update discord_users set banned = 1 where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP1(user_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}
DiagnosticSQLResult<bool> UnbanUser(QSqlDatabase db, QString user_id){
    QString qs = "update discord_users set banned = 0 where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP1(user_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> UpdateCurrentPage(QSqlDatabase db, QString user_id, int page)
{
    QString qs = "update user_lists set at_page = :page where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP2(user_id, page));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> UnfilterFandom(QSqlDatabase db, QString user_id, int fandom_id)
{
    QString qs = "delete from filtered_fandoms where user_id = :user_id and fandom_id = :fandom_id";
    SqlContext<bool> ctx(db, qs, BP2(user_id, fandom_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> FilterFandom(QSqlDatabase db, QString user_id, int fandom_id, bool including_crossovers)
{
    QString qs = "insert into filtered_fandoms(user_id, fandom_id, including_crossovers) values(:user_id, :fandom_id, :including_crossovers)";
    SqlContext<bool> ctx(db, qs, BP3(user_id, fandom_id, including_crossovers));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> ResetFandomFilter(QSqlDatabase db, QString user_id)
{
    QString qs = "delete from filtered_fandoms where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP1(user_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> ResetFandomIgnores(QSqlDatabase db, QString user_id)
{
    QString qs = "delete from ignored_fandoms where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP1(user_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> ResetFicIgnores(QSqlDatabase db, QString user_id)
{
    QString qs = "delete from fic_tags where user_id = :user_id and tag = 'ignore'";
    SqlContext<bool> ctx(db, qs, BP1(user_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteUserFFNId(QSqlDatabase db, QString user_id, int ffn_id)
{
    QString qs = "update discord_users set ffn_id = :ffn_id where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP2(user_id, ffn_id));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> FillUserUids(QSqlDatabase db)
{
    QString qs = QString("select user_id from discord_users where uuid is null");

    SqlContext<QSet<QString>> userFetcher(db, qs);
    userFetcher.FetchLargeSelectIntoList<QString>("user_id",qs);
    auto list = userFetcher.result.data;

    for(auto user_id : list){
        qs = QString("update discord_users set uuid = :uuid where user_id = :user_id");
        auto uuid = QUuid::createUuid().toString();
        QLOG_INFO() << "Updating user: " << user_id << " with uuid: " << uuid;
        SqlContext<QSet<int>> userUpdater(db, qs, BP2(uuid, user_id));
        userUpdater.ExecAndCheck(true);
    }
    DiagnosticSQLResult<bool> result;
    result.data = true;
    return result;
}

DiagnosticSQLResult<bool> WriteForcedListParams(QSqlDatabase db, QString user_id, int forced_min_matches, int forced_ratio)
{
    QString qs = "update discord_users set forced_min_matches = :forced_min_matches, forced_ratio = :forced_ratio where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP3(user_id, forced_min_matches, forced_ratio));
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteForceLikedAuthors(QSqlDatabase db, QString user_id, bool use_liked_authors_only)
{
    QString qs = "update discord_users set use_liked_authors_only = :use_liked_authors_only where user_id = :user_id";
    SqlContext<bool> ctx(db, qs, BP2(user_id, use_liked_authors_only));
    ctx.ExecAndCheck(true);
    return ctx.result;
}













}
}

