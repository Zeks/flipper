#include "discord/task_environment.h"
#include "include/grpc/grpc_source.h"
#include "include/sqlitefunctions.h"

#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"
#include <QSettings>


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
    QSharedPointer<database::IDBWrapper> userDbInterface (new database::SqliteInterface());
    userDbInterface->SetDatabase(database::sqlite::InitAndUpdateDatabaseForFile("database","DiscordDB","dbcode/discord_init.sql", "DiscordDB", false));
    userDbInterface->EnsureUUIDForUserDatabase();

    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    auto pageCacheDb = pageCacheInterface->InitDatabase2("PageCache","pcInit");
    pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "pcInit");

    this->userDbInterface = userDbInterface;
    authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    fandoms = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandoms->isClient = true;
    fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    authors->portableDBInterface = userDbInterface;
    fanfics->authorInterface = authors;
    fanfics->fandomInterface = fandoms;
    fandoms->portableDBInterface = userDbInterface;
    authors->db = userDbInterface->GetDatabase();
    fanfics->db = userDbInterface->GetDatabase();
    fandoms->db = userDbInterface->GetDatabase();
    userDbInterface->userToken = userDbInterface->GetUserToken();

    QSettings settings("settings/settings_discord.ini", QSettings::IniFormat);
    auto ip = settings.value("Login/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Login/serverPort", "3055").toString();
    ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), userDbInterface->userToken,  160));
}

}
