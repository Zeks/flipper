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
#include "discord/task_environment.h"
#include "include/grpc/grpc_source.h"
#include "include/sqlitefunctions.h"

#include "Interfaces/db_interface.h"

#include "Interfaces/fanfics.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"
#include "include/sqlitefunctions.h"
#include <QSettings>
#include "sqlitefunctions.h"


#include "GlobalHeaders/SingletonHolder.h"
namespace discord {
static inline QString CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    std::string result = server_address_proto.arg(ip,port).toStdString();
    return QString::fromStdString(result);
}


void TaskEnvironment::Init()
{
    // todo check if it's executing properly
    // not sure why sqlite and plsql somehow coexist
    this->db = database::sqlite::InitAndUpdateSqliteDatabaseForFile("database","DiscordDB","dbcode/discord_init.sql", "DiscordDB", false);

    //todo - why am I reading these two databases by differnt methods? check!
    auto pageCacheDb = database::sqlite::InitSqliteDatabase2("PageCache","pcInit");
    database::sqlite::ReadDbFile("dbcode/pagecacheinit.sql", "pcInit");

    authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    fandoms = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandoms->isClient = true;
    fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    authors->db = this->db;
    fanfics->authorInterface = authors;
    fanfics->fandomInterface = fandoms;
    fandoms->db = this->db;
    authors->db = this->db;
    fanfics->db = this->db;
    fandoms->db = this->db;

    QSettings settings("settings/settings_discord.ini", QSettings::IniFormat);
    auto ip = settings.value("Login/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Login/serverPort", "3055").toString();
    static const int defaultDeadline = 160;
    // todo I do need to store user token extrenally upon init, just not in such a bad calss it was in
    ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port),
                                      sql::GetUserToken(db).data,
                                      defaultDeadline));
}

}
