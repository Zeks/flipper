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
    std::string qs = "select * from discord_users where user_id = :user_id";

    SqlContext<QSharedPointer<discord::User>> ctx(db, std::move(qs), BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<discord::User> user(new discord::User);
        user->SetUserID(user_id);
        user->SetUserName(q.value(QStringLiteral("user_name")).toString());
        user->SetFfnID(q.value(QStringLiteral("ffn_id")).toString());
        user->SetCurrentListId(q.value(QStringLiteral("current_list")).toInt());
        user->SetBanned(q.value(QStringLiteral("banned")).toBool());
        user->SetUseLikedAuthorsOnly(q.value(QStringLiteral("use_liked_authors_only")).toBool());
        user->SetUuid(q.value(QStringLiteral("uuid")).toString());
        user->SetSortFreshFirst(q.value(QStringLiteral("use_fresh_sorting")).toInt());
        user->SetHideDead(q.value(QStringLiteral("hide_dead")).toInt());

        auto minWords = static_cast<uint64_t>(q.value(QStringLiteral("words_filter_range_begin")).toULongLong());
        auto maxWords = static_cast<uint64_t>(q.value(QStringLiteral("words_filter_range_end")).toULongLong());
        user->SetWordcountFilter({minWords, maxWords});
        user->SetShowCompleteOnly(q.value(QStringLiteral("show_complete_only")).toInt());
        user->SetStrictFreshSort(q.value(QStringLiteral("strict_fresh_sorting")).toInt());
        discord::LargeListToken token;
        token.date = QDate::fromString(q.value(QStringLiteral("last_large_list_generated")).toString(), "yyyyMMdd");
        token.counter = q.value(QStringLiteral("last_large_list_counter")).toInt();
        user->SetLargeListToken(token);
        ctx.result.data = user;
    });
    if(!ctx.result.data)
        return std::move(ctx.result);;
    std::string page = "select at_page from user_lists where user_id = :user_id";
    SqlContext<int> ctxPage(db, std::move(page), BP1(user_id));
    ctxPage.FetchSingleValue<int>("at_page", 0);
    ctx.result.data->AdvancePage(ctxPage.result.data, false);
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSharedPointer<discord::Server>> GetServer(QSqlDatabase db, const std::string& serverId){
    std::string qs = "select * from discord_servers where server_id = :server_id";

    QString server_id = QString::fromStdString(serverId);
    SqlContext<QSharedPointer<discord::Server>> ctx(db, std::move(qs), BP1(server_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<discord::Server> server(new discord::Server);
        server->SetServerId(serverId);

        server->SetBanned(q.value(QStringLiteral("server_banned")).toBool());
        server->SetSilenced(q.value(QStringLiteral("server_silenced")).toBool());
        server->SetAnswerInPm(q.value(QStringLiteral("bot_answers_in_pm")).toBool());

        server->SetParserRequestLimit(q.value(QStringLiteral("parse_request_limit")).toInt());
        server->SetTotalRequests(q.value(QStringLiteral("total_requests")).toInt());

        server->SetFirstActive(q.value(QStringLiteral("active_since")).toDateTime());
        server->SetLastActive(q.value(QStringLiteral("last_request")).toDateTime());


        server->SetOwnerId(q.value(QStringLiteral("owner_id")).toString());
        server->SetDedicatedChannelId(q.value(QStringLiteral("dedicated_channel_id")).toString().toStdString());
        server->SetCommandPrefix(q.value(QStringLiteral("command_prefix")).toString().toStdString());
        server->SetServerName(q.value(QStringLiteral("server_name")).toString());

        ctx.result.data = server;
    });
    return std::move(ctx.result);
}



DiagnosticSQLResult<discord::FandomFilter> GetFandomIgnoreList(QSqlDatabase db, QString user_id){
    std::string qs = "select * from ignored_fandoms where user_id = :user_id";

    SqlContext<discord::FandomFilter> ctx(db, std::move(qs), BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.AddFandom(q.value(QStringLiteral("fandom_id")).toInt(),
                                  q.value(QStringLiteral("including_crossovers")).toBool());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<discord::FandomFilter> GetFilterList(QSqlDatabase db, QString user_id){
    std::string qs = "select * from filtered_fandoms where user_id = :user_id";

    SqlContext<discord::FandomFilter> ctx(db, std::move(qs), BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.AddFandom(q.value(QStringLiteral("fandom_id")).toInt(),
                                  q.value(QStringLiteral("including_crossovers")).toBool());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSet<int>> GetFicIgnoreList(QSqlDatabase db, QString user_id)
{
    std::string qs = "select fic_id from fic_tags where user_id = :user_id and fic_tag = 'ignored'";

    SqlContext<QSet<int>> ctx(db,  BP1(user_id));
    ctx.FetchLargeSelectIntoList<int>("fic_id",std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<int> GetCurrentPage(QSqlDatabase db, QString user_id)
{
    std::string qs = "select at_page from user_lists where user_id = :user_id";

    SqlContext<int> ctx(db, std::move(qs), BP1(user_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = q.value(QStringLiteral("at_page")).toInt();
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteUser(QSqlDatabase db, QSharedPointer<discord::User> user){
    std::string qs = "INSERT INTO discord_users(user_id, user_name, ffn_id, reads_slash, current_list, banned, uuid) values(:user_id, :user_name, :ffn_id, :reads_slash, 0, 0, :uuid)";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("user_id", user->UserID());
    ctx.bindValue("user_name", user->UserName());
    ctx.bindValue("ffn_id", user->FfnID());
    ctx.bindValue("reads_slash", user->ReadsSlash());
    ctx.bindValue("uuid", user->GetUuid());
    ctx.ExecAndCheck(false);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteServer(QSqlDatabase db, QSharedPointer<discord::Server> server)
{
    std::string qs = "INSERT INTO discord_servers(server_id, server_name, active_since, last_request) "
                 "values(:server_id, :server_name, :active_since, :last_request)";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("server_id", QString::fromStdString(server->GetServerId()));
    ctx.bindValue("server_name", server->GetServerName());
    ctx.bindValue("active_since", server->GetFirstActive());
    ctx.bindValue("last_request", server->GetLastActive());
    ctx.ExecAndCheck(false);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteServerPrefix(QSqlDatabase db, const std::string& serverId, QString new_prefix)
{
    std::string qs = "update discord_servers set command_prefix = :new_prefix where server_id = :server_id";
    QString server_id = QString::fromStdString(serverId);
    SqlContext<bool> ctx(db, std::move(qs), BP2(server_id, new_prefix));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteServerDedicatedChannel(QSqlDatabase db, const std::string &serverId, const std::string & dedicatedChannelI)
{
    std::string qs = "update discord_servers set dedicated_channel_id = :dedicated_channel_id where server_id = :server_id";
    QString server_id = QString::fromStdString(serverId);
    QString dedicated_channel_id = QString::fromStdString(dedicatedChannelI);
    SqlContext<bool> ctx(db, std::move(qs), BP2(server_id, dedicated_channel_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<int> WriteUserList(QSqlDatabase db, QString user_id, QString list_name,
                                       discord::EListType list_type,
                                       int min_match, int match_ratio, int always_at)
{
    std::string qs = "INSERT INTO user_lists(user_id, list_name, list_type, min_match, match_ratio, always_at,  list_id, generated)"
                 " values(:user_id, :list_name, :list_type, :min_match, :match_ratio, :always_at, :list_id, :generated)";
    int list_id = 0;
    auto generated= QDateTime::currentDateTimeUtc();
    SqlContext<bool> ctx(db, std::move(qs), BP8(user_id, list_name, list_type, min_match, match_ratio, always_at, list_id, generated));
    ctx.ExecAndCheck(false);
    DiagnosticSQLResult<int> result;
    result.data = 0;
    result.success = ctx.result.success;
    result.oracleError = ctx.result.oracleError;
    return result;
}

DiagnosticSQLResult<bool> DeleteUserList(QSqlDatabase db, QString user_id, QString list_name)
{
    std::string qs = "delete from user_lists where user_id = :user_id and list_id = 0";
    //auto generated= QDateTime::currentDateTimeUtc();
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, list_name));
    ctx.ExecAndCheck(false);
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> IgnoreFandom(QSqlDatabase db, QString user_id, int fandom_id, bool including_crossovers){
    UnignoreFandom(db, user_id, fandom_id);
    std::string qs = "INSERT INTO ignored_fandoms(user_id, fandom_id,including_crossovers) values(:user_id, :fandom_id, :including_crossovers)";
    SqlContext<bool> ctx(db, std::move(qs), BP3(user_id, fandom_id, including_crossovers));
    ctx.ExecAndCheck(false);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> UnignoreFandom(QSqlDatabase db, QString user_id, int fandom_id){
    std::string qs = "delete from ignored_fandoms where user_id= :user_id and fandom_id = :fandom_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, fandom_id));
    ctx.ExecAndCheck(false);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> TagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag){
    std::string qs = "INSERT INTO fic_tags(user_id, fic_id,fic_tag) values(:user_id, :fic_id, :fic_tag)";
    SqlContext<bool> ctx(db, std::move(qs), BP3(user_id, fic_id, fic_tag));
    ctx.ExecAndCheck(false);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> UnTagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag){
    if(fic_tag.isEmpty())
    {
        std::string qs = "delete from fic_tags where user_id= :user_id and fic_id = :fic_id";
        SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, fic_id));
        ctx.ExecAndCheck(false);
        return std::move(ctx.result);
    }
    else {
        std::string qs = "delete from fic_tags where user_id= :user_id and fic_id = :fic_id and fic_tag = :fic_tag";
        SqlContext<bool> ctx(db, std::move(qs), BP3(user_id, fic_id, fic_tag));
        ctx.ExecAndCheck(false);
        return std::move(ctx.result);
    }

}
DiagnosticSQLResult<bool> BanUser(QSqlDatabase db, QString user_id){
    std::string qs = "update discord_users set banned = 1 where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> UnbanUser(QSqlDatabase db, QString user_id){
    std::string qs = "update discord_users set banned = 0 where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> UpdateCurrentPage(QSqlDatabase db, QString user_id, int page)
{
    std::string qs = "update user_lists set at_page = :page where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, page));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> UnfilterFandom(QSqlDatabase db, QString user_id, int fandom_id)
{
    std::string qs = "delete from filtered_fandoms where user_id = :user_id and fandom_id = :fandom_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, fandom_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> FilterFandom(QSqlDatabase db, QString user_id, int fandom_id, bool including_crossovers)
{
    std::string qs = "insert into filtered_fandoms(user_id, fandom_id, including_crossovers) values(:user_id, :fandom_id, :including_crossovers)";
    SqlContext<bool> ctx(db, std::move(qs), BP3(user_id, fandom_id, including_crossovers));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> ResetFandomFilter(QSqlDatabase db, QString user_id)
{
    std::string qs = "delete from filtered_fandoms where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> ResetFandomIgnores(QSqlDatabase db, QString user_id)
{
    std::string qs = "delete from ignored_fandoms where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> ResetFicIgnores(QSqlDatabase db, QString user_id)
{
    std::string qs = "delete from fic_tags where user_id = :user_id and tag = 'ignore'";
    SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteUserFFNId(QSqlDatabase db, QString user_id, int ffn_id)
{
    std::string qs = "update discord_users set ffn_id = :ffn_id where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, ffn_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteUserFavouritesSize(QSqlDatabase db, QString user_id, int favourites_size)
{
    std::string qs = "update discord_users set favourites_size = :favourites_size where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, favourites_size));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> FillUserUids(QSqlDatabase db)
{
    std::string qs = "select user_id from discord_users where uuid is null";

    SqlContext<QSet<QString>> userFetcher(db);
    userFetcher.FetchLargeSelectIntoList<QString>("user_id",std::move(qs));
    auto list = userFetcher.result.data;

    for(const auto& user_id : std::as_const(list)){
        qs = "update discord_users set uuid = :uuid where user_id = :user_id";
        auto uuid = QUuid::createUuid().toString();
        QLOG_INFO() << "Updating user: " << user_id << " with uuid: " << uuid;
        SqlContext<QSet<int>> userUpdater(db, std::move(qs), BP2(uuid, user_id));
        userUpdater.ExecAndCheck(true);
    }
    DiagnosticSQLResult<bool> result;
    result.data = true;
    return result;
}

DiagnosticSQLResult<bool> WriteForcedListParams(QSqlDatabase db, QString user_id, int forced_min_matches, int forced_ratio)
{
    std::string qs = "update discord_users set forced_min_matches = :forced_min_matches, forced_ratio = :forced_ratio where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP3(user_id, forced_min_matches, forced_ratio));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteForceLikedAuthors(QSqlDatabase db, QString user_id, bool use_liked_authors_only)
{
    std::string qs = "update discord_users set use_liked_authors_only = :use_liked_authors_only where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, use_liked_authors_only));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteFreshSortingParams(QSqlDatabase db, QString user_id, bool use_fresh_sorting, bool strict_fresh_sorting)
{
    std::string qs = "update discord_users set use_fresh_sorting = :use_fresh_sorting, strict_fresh_sorting = :strict_fresh_sorting where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP3(user_id, use_fresh_sorting, strict_fresh_sorting));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteLargeListReparseToken(QSqlDatabase db, QString user_id, discord::LargeListToken token)
{
    std::string qs = "update discord_users set last_large_list_generated = :last_large_list_generated, last_large_list_counter = :last_large_list_counter where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("user_id", user_id);
    ctx.bindValue("last_large_list_generated", token.date.toString("yyyyMMdd"));
    ctx.bindValue("last_large_list_counter", token.counter);
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> SetHideDeadFilter(QSqlDatabase db, QString user_id, bool hide_dead)
{
    std::string qs = "update discord_users set hide_dead = :hide_dead where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, hide_dead));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> SetCompleteFilter(QSqlDatabase db, QString user_id, bool show_complete_only)
{
    std::string qs = "update discord_users set show_complete_only = :show_complete_only  where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, show_complete_only));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> CompletelyRemoveUser(QSqlDatabase db, QString user_id)
{
    if(user_id.isEmpty()){
        DiagnosticSQLResult<bool> falseResult;
        return falseResult;
    }
    {
        std::string qs = "delete from discord_users where user_id = :user_id";
        SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
        ctx.ExecAndCheck(true);
    }
    {
        std::string qs = "delete from fic_tags where user_id = :user_id";
        SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
        ctx.ExecAndCheck(true);
    }
    {
        std::string qs = "delete from filtered_fandoms  where user_id = :user_id";
        SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
        ctx.ExecAndCheck(true);
    }
    {
        std::string qs = "delete from ignored_fandoms where user_id = :user_id";
        SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
        ctx.ExecAndCheck(true);
    }

    std::string qs = "delete from user_lists where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(user_id));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> SetWordcountFilter(QSqlDatabase db, QString userId, discord::WordcountFilter filter)
{
    std::string qs = "update discord_users set words_filter_range_begin = :words_filter_range_begin, words_filter_range_end = :words_filter_range_end where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("user_id", userId);
    ctx.bindValue("words_filter_range_begin", QVariant::fromValue<long long>(filter.firstLimit));
    ctx.bindValue("words_filter_range_end", QVariant::fromValue<long long>(filter.secondLimit));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> SetDeadFicDaysRange(QSqlDatabase db, QString user_id, int dead_fic_days_range)
{
    std::string qs = "update discord_users set dead_fic_days_range = :dead_fic_days_range where user_id = :user_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(user_id, dead_fic_days_range));
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}






}
}

