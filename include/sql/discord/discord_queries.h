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
#include "sqlcontext.h"
namespace database {
using namespace puresql;
namespace discord_quries{

    DiagnosticSQLResult<QSharedPointer<discord::User>> GetUser(QSqlDatabase db, QString);
    DiagnosticSQLResult<QList<discord::IgnoreFandom>> GetIgnoreList(QSqlDatabase db, QString userId);

    DiagnosticSQLResult<bool> WriteUser(QSqlDatabase db, QSharedPointer<discord::User>);
    DiagnosticSQLResult<bool> DeleteUserList(QSqlDatabase db, QString user_id, QString list_name);
    DiagnosticSQLResult<int> WriteUserList(QSqlDatabase db, QString user_id,
                                           QString list_name, discord::EListType list_type,
                                           int min_match, int match_ratio, int always_at);
    DiagnosticSQLResult<bool> IgnoreFandom(QSqlDatabase db, QString user_id, int fandom_id, bool including_crossovers);
    DiagnosticSQLResult<bool> UnignoreFandom(QSqlDatabase db, QString userId, int fandomId);
    DiagnosticSQLResult<bool> TagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag);
    DiagnosticSQLResult<bool> UnTagFanfic(QSqlDatabase db, QString user_id, int fic_id, QString fic_tag); // empty tag removes all
    DiagnosticSQLResult<bool> BanUser(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> UnbanUser(QSqlDatabase db, QString user_id);
    DiagnosticSQLResult<bool> UpdateCurrentPage(QSqlDatabase db, QString user_id, int page);
 }
}
