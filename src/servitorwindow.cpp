#include "include/servitorwindow.h"
#include "include/Interfaces/genres.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/interface_sqlite.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/Interfaces/ffn/ffn_fanfics.h"
#include "include/Interfaces/ffn/ffn_authors.h"
#include "include/url_utils.h"
#include "include/favholder.h"
#include "include/tasks/slash_task_processor.h"
#include <QTextCodec>
#include <QSettings>
#include <QSqlRecord>
#include <QFuture>
#include <QtConcurrent>
#include <QFutureWatcher>
#include "ui_servitorwindow.h"
#include "include/parsers/ffn/ficparser.h"
#include "include/parsers/ffn/favparser.h"
#include "include/timeutils.h"
#include "include/page_utils.h"
#include "pagegetter.h"
#include "tasks/recommendations_reload_precessor.h"



ServitorWindow::ServitorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::servitorWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<WebPage>("WebPage");
    qRegisterMetaType<PageResult>("PageResult");
    qRegisterMetaType<ECacheMode>("ECacheMode");
    //    qRegisterMetaType<FandomParseTask>("FandomParseTask");
    //    qRegisterMetaType<FandomParseTaskResult>("FandomParseTaskResult");
    ReadSettings();
}

ServitorWindow::~ServitorWindow()
{
    WriteSettings();
    delete ui;
}

void ServitorWindow::ReadSettings()
{
    QSettings settings("servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    ui->leFicUrl->setText(settings.value("Settings/ficUrl", "").toString());
    ui->leReprocessFics->setText(settings.value("Settings/reprocessIds", "").toString());
}

void ServitorWindow::WriteSettings()
{
    QSettings settings("servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(!ui->leFicUrl->text().trimmed().isEmpty())
        settings.setValue("Settings/ficUrl", ui->leFicUrl->text());
    if(!ui->leReprocessFics->text().trimmed().isEmpty())
        settings.setValue("Settings/reprocessIds", ui->leReprocessFics->text());
    settings.sync();
}

void ServitorWindow::UpdateInterval(int, int)
{

}

static QHash<QString, int> CreateGenreRedirects(){
    QHash<QString, int> result;
    result["General"] = 0;
    result["Humor"] = 1;
    result["Poetry"] = 2;
    result["Adventure"] = 3;
    result["Mystery"] = 4;
    result["Horror"] = 5;
    result["Parody"] = 6;
    result["Angst"] = 7;
    result["Supernatural"] = 8;
    result["Suspense"] = 9;
    result["Romance"] = 10;
    result["not found"] = 11;
    result["Sci-Fi"] = 13;
    result["Fantasy"] = 14;
    result["Spiritual"] = 15;
    result["Tragedy"] = 16;
    result["Western"] = 17;
    result["Crime"] = 18;
    result["Family"] = 19;
    result["Hurt/Comfort"] = 20;
    result["Friendship"] = 21;
    result["Drama"] = 22;
    return result;
}
void ServitorWindow::DetectGenres(int minAuthorRecs, int minFoundLists)
{
    interfaces::GenreConverter converter;



    QVector<int> ficIds;
    auto db = QSqlDatabase::database();
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authors= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    genres->db = db;
    fanfics->db = db;
    authors->db = db;




    An<core::FavHolder> holder;
    holder->LoadFavourites(authors);
    qDebug() << "Finished list load";

    QHash<int, QSet<int>> ficsToUse;

    for(int key : holder->favourites.keys())
    {
        auto& set = holder->favourites[key];
        //qDebug() <<  key << " set size is: " << set.size();
        if(set.size() < minAuthorRecs)
            continue;
        //qDebug() << "processing";
        for(auto fic : set)
            ficsToUse[fic].insert(key);
    }
    qDebug() << "Finished author processing, resulting set is of size:" << ficsToUse.size();



    QList<int> result;
    for(auto key : ficsToUse.keys())
    {
        if(ficsToUse[key].size() >= minFoundLists)
            result.push_back(key);

    }
    qDebug() << "Finished counts";
    database::Transaction transaction(db);
    //    for(auto fic: result)
    //        fanfics->AssignQueuedForFic(fic);

    auto genreLists = authors->GetListGenreData();
    qDebug() << "got genre lists, size: " << genreLists.size();

    auto genresForFics = fanfics->GetGenreForFics();
    qDebug() << "collected genres for fics, size: " << genresForFics.size();
    auto moodLists = authors->GetMoodDataForLists();
    qDebug() << "got mood lists, size: " << moodLists.size();

    auto genreRedirects = CreateGenreRedirects();

    QVector<genre_stats::FicGenreData> ficGenreDataList;
    ficGenreDataList.reserve(ficsToUse.keys().size());
    interfaces::GenreConverter genreConverter;
    int counter = 0;
    for(auto fic : result)
    {
        if(counter%10000 == 0)
            qDebug() << "processing fic: " << counter;

        //gettting amount of funny lists
        int64_t funny = std::count_if(std::begin(ficsToUse[fic]), std::end(ficsToUse[fic]), [&moodLists](int listId){
            return moodLists[listId].strengthFunny >= 0.3f;
        });
        int64_t flirty = std::count_if(ficsToUse[fic].begin(), ficsToUse[fic].end(), [&](int listId){
            return moodLists[listId].strengthFlirty >= 0.5f;
        });
        auto listSet = ficsToUse[fic];
        int64_t neutralAdventure = 0;
        for(auto listId : listSet)
            if(genreLists[listId][3] >= 0.3)
                neutralAdventure++;

        //qDebug() << "Adventure list count: " << neutralAdventure;

        int64_t hurty = std::count_if(ficsToUse[fic].begin(), ficsToUse[fic].end(), [&](int listId){
            return moodLists[listId].strengthHurty>= 0.15f;
        });
        int64_t bondy = std::count_if(ficsToUse[fic].begin(), ficsToUse[fic].end(), [&](int listId){
            return moodLists[listId].strengthBondy >= 0.3f;
        });

        int64_t neutral = std::count_if(ficsToUse[fic].begin(), ficsToUse[fic].end(), [&](int listId){
            return moodLists[listId].strengthNeutral>= 0.3f;
        });
        int64_t dramatic = std::count_if(ficsToUse[fic].begin(), ficsToUse[fic].end(), [&](int listId){
            return moodLists[listId].strengthDramatic >= 0.3f;
        });

        int64_t total = ficsToUse[fic].size();

        genre_stats::FicGenreData genreData;
        genreData.ficId = fic;
        genreData.originalGenres =  genreConverter.GetFFNGenreList(genresForFics[fic]);
        genreData.totalLists = static_cast<int>(total);
        genreData.strengthHumor = static_cast<float>(funny)/static_cast<float>(total);
        genreData.strengthRomance = static_cast<float>(flirty)/static_cast<float>(total);
        genreData.strengthDrama = static_cast<float>(dramatic)/static_cast<float>(total);
        genreData.strengthBonds = static_cast<float>(bondy)/static_cast<float>(total);
        genreData.strengthHurtComfort = static_cast<float>(hurty)/static_cast<float>(total);
        genreData.strengthNeutralComposite = static_cast<float>(neutral)/static_cast<float>(total);
        genreData.strengthNeutralAdventure = static_cast<float>(neutralAdventure)/static_cast<float>(total);
        //        qDebug() << "Calculating adventure value: " << "Adventure lists: " <<  neutralAdventure << " total lists: " << total << " fic: " << fic;
        //        qDebug() << "Calculated value: " << genreData.strengthNeutralAdventure;
        genreConverter.ProcessGenreResult(genreData);
        ficGenreDataList.push_back(genreData);
        counter++;
    }

    if(!genres->WriteDetectedGenres(ficGenreDataList))
        transaction.cancel();
    qDebug() << "finished writing genre data for fics";
    transaction.finalize();
    qDebug() << "Finished queue set";
}
//        int64_t pureDrama = std::count_if(genreLists[fic].begin(), genreLists[fic].end(), [&](int listId){
//            return genreLists[listId][static_cast<size_t>(genreRedirects["Drama"])] - genreLists[listId][static_cast<size_t>(genreRedirects["Romance"])] >= 0.05;
//        });
//        int64_t pureRomance = std::count_if(genreLists[fic].begin(), genreLists[fic].end(), [&](int listId){
//            return genreLists[listId][static_cast<size_t>(genreRedirects["Romance"])] - genreLists[listId][static_cast<size_t>(genreRedirects["Drama"])] >= 0.8;
//        });


void ServitorWindow::on_pbLoadFic_clicked()
{
    //    PageManager pager;
    //    FicParser parser;
    //    QHash<QString, int> fandoms;
    //    auto result = database::GetAllFandoms(fandoms);
    //    if(!result)
    //        return;
    //    QString url = ui->leFicUrl->text();
    //    auto page = pager.GetPage(url, ECacheMode::use_cache);
    //    parser.SetRewriteAuthorNames(false);
    //    parser.ProcessPage(url, page.content);
    //    parser.WriteProcessed(fandoms);
}

void ServitorWindow::on_pbReprocessFics_clicked()
{
    //    PageManager pager;
    //    FicParser parser;
    //    QHash<QString, int> fandoms;
    //    auto result = database::GetAllFandoms(fandoms);
    //    if(!result)
    //        return;
    //    QSqlDatabase db = QSqlDatabase::database();
    //    //db.transaction();
    //    database::ReprocessFics(" where fandom1 like '% CROSSOVER' and alive = 1 order by id asc", "ffn", [this,&pager, &parser, &fandoms](int ficId){
    //        //todo, get web_id from fic_id
    //        QString url = url_utils::GetUrlFromWebId(webId, "ffn");
    //        auto page = pager.GetPage(url, ECacheMode::use_only_cache);
    //        parser.SetRewriteAuthorNames(false);
    //        auto fic = parser.ProcessPage(url, page.content);
    //        if(fic.isValid)
    //            parser.WriteProcessed(fandoms);
    //    });
}

void ServitorWindow::on_pushButton_clicked()
{
    //database::EnsureFandomsNormalized();
}

void ServitorWindow::on_pbGetGenresForFic_clicked()
{

}

void ServitorWindow::on_pbGetGenresForEverything_clicked()
{
    DetectGenres(25,15);
    interfaces::GenreConverter converter;

    //    QVector<int> ficIds;
    //    auto db = QSqlDatabase::database();
    //    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    //    genres->db = db;
    //    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    //    fanfics->db = db;

    //    QSettings settings("settings_servitor.ini", QSettings::IniFormat);
    //    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    //    bool alreadyQueued = settings.value("Settings/genrequeued", false).toBool();
    //    if(!alreadyQueued)
    //    {
    //        database::Transaction transaction(db);
    //        genres->QueueFicsForGenreDetection(25, 15, 0);
    //        settings.setValue("Settings/genrequeued", true);
    //        settings.sync();
    //        transaction.finalize();
    //    }
    //    database::Transaction transaction(db);
    //    qDebug() << "reading genre data for fics";
    //    auto genreData = genres->GetGenreDataForQueuedFics();
    //    qDebug() << "finished reading genre data for fics";
    //    for(auto& fic : genreData)
    //    {
    //        converter.ProcessGenreResult(fic);
    //    }
    //    qDebug() << "finished processing genre data for fics";
    //    if(!genres->WriteDetectedGenres(genreData))
    //        transaction.cancel();
    //    qDebug() << "finished writing genre data for fics";
    //    transaction.finalize();
}

void ServitorWindow::on_pushButton_2_clicked()
{
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanfics->db = QSqlDatabase::database();
    fanfics->ResetActionQueue();
    QSettings settings("settings_servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.setValue("Settings/genrequeued", false);
    settings.sync();
}

void ServitorWindow::on_pbGetData_clicked()
{
    An<PageManager> pager;
    auto data = pager->GetPage(ui->leGetCachedData->text(), ECacheMode::use_only_cache);
    ui->edtLog->clear();
    ui->edtLog->insertPlainText(data.content);

    // need to get:
    // last date of published favourite fic
    // amount of favourite fics at the moment of last parse
    // (need to keep this to check if list is updated even if last published is teh same)
}

void ServitorWindow::on_pushButton_3_clicked()
{
    auto db = QSqlDatabase::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanfics->db = db;
    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    authorInterface->db = db;
    auto authors = authorInterface->GetAllAuthorsLimited("ffn", 0);


    An<PageManager> pager;

    auto job = [fanfics, authorInterface, fandomInterface](QString url, QString content){
        FavouriteStoryParser parser(fanfics);
        parser.ProcessPage(url, content);
        return parser;
    };

    QList<QFuture<FavouriteStoryParser>> futures;
    QList<FavouriteStoryParser> parsers;



    database::Transaction transaction(db);
    WebPage data;
    int counter = 0;
    for(auto author : authors)
    {
        if(counter%1000 == 0)
            QLOG_INFO() <<  counter;

        futures.clear();
        parsers.clear();
        //QLOG_INFO() <<  "Author: " << author->url("ffn");
        FavouriteStoryParser parser(fanfics);
        parser.authorName = author->name;

        //TimedAction pageAction("Page loaded in: ",[&](){
        data = pager->GetPage(author->url("ffn"), ECacheMode::use_only_cache);
        //});
        //pageAction.run();

        //TimedAction pageProcessAction("Page processed in: ",[&](){
        auto splittings = page_utils::SplitJob(data.content);

        for(auto part: splittings.parts)
        {
            futures.push_back(QtConcurrent::run(job, data.url, part.data));
        }
        for(auto future: futures)
        {
            future.waitForFinished();
            parsers+=future.result();
        }

        //});
        //pageProcessAction.run();

        QSet<QString> fandoms;
        FavouriteStoryParser::MergeStats(author, fandomInterface, parsers);
        //TimedAction action("Author updated in: ",[&](){
        authorInterface->UpdateAuthorFavouritesUpdateDate(author);
        //});
        //action.run();
        //QLOG_INFO() <<  "Author: " << author->url("ffn") << " Update date: " << author->stats.favouritesLastUpdated;
        counter++;

    }
    transaction.finalize();

}

void ServitorWindow::on_pbUpdateFreshAuthors_clicked()
{
    auto db = QSqlDatabase::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    authorInterface->portableDBInterface = dbInterface;

    auto authors = authorInterface->GetAllAuthorsWithFavUpdateSince("ffn", QDateTime::currentDateTime().addMonths(-24));



    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;
    fandomInterface->portableDBInterface = dbInterface;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->portableDBInterface = dbInterface;
    recsInterface->authorInterface = authorInterface;



    RecommendationsProcessor reloader(db, fanficsInterface,
                                      fandomInterface,
                                      authorInterface,
                                      recsInterface);

    connect(&reloader, &RecommendationsProcessor::resetEditorText, this,    &ServitorWindow::OnResetTextEditor);
    connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &ServitorWindow::OnProgressBarRequested);
    connect(&reloader, &RecommendationsProcessor::updateCounter, this,      &ServitorWindow::OnUpdatedProgressValue);
    connect(&reloader, &RecommendationsProcessor::updateInfo, this,         &ServitorWindow::OnNewProgressString);


    reloader.SetStagedAuthors(authors);
    reloader.ReloadRecommendationsList(ECacheMode::use_cache);

}

void ServitorWindow::OnResetTextEditor()
{

}

void ServitorWindow::OnProgressBarRequested()
{

}

void ServitorWindow::OnUpdatedProgressValue(int )
{

}

void ServitorWindow::OnNewProgressString(QString )
{

}

void ServitorWindow::on_pbUnpdateInterval_clicked()
{
    auto db = QSqlDatabase::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    authorInterface->portableDBInterface = dbInterface;

    auto authors = authorInterface->GetAllAuthorsWithFavUpdateBetween("ffn",
                                                                      QDateTime::currentDateTime().addMonths(-1*ui->sbUpdateIntervalStart->value()),
                                                                      QDateTime::currentDateTime().addMonths(-1*ui->sbUpdateIntervalEnd->value())

                                                                      );



    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;
    fandomInterface->portableDBInterface = dbInterface;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->portableDBInterface = dbInterface;
    recsInterface->authorInterface = authorInterface;



    RecommendationsProcessor reloader(db, fanficsInterface,
                                      fandomInterface,
                                      authorInterface,
                                      recsInterface);

    connect(&reloader, &RecommendationsProcessor::resetEditorText, this,    &ServitorWindow::OnResetTextEditor);
    connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &ServitorWindow::OnProgressBarRequested);
    connect(&reloader, &RecommendationsProcessor::updateCounter, this,      &ServitorWindow::OnUpdatedProgressValue);
    connect(&reloader, &RecommendationsProcessor::updateInfo, this,         &ServitorWindow::OnNewProgressString);


    reloader.SetStagedAuthors(authors);
    reloader.ReloadRecommendationsList(ECacheMode::use_cache);
}

void ServitorWindow::on_pbReprocessAllFavPages_clicked()
{
    auto db = QSqlDatabase::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    authorInterface->portableDBInterface = dbInterface;

    auto authors = authorInterface->GetAllAuthors("ffn");


    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;
    fandomInterface->portableDBInterface = dbInterface;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->portableDBInterface = dbInterface;
    recsInterface->authorInterface = authorInterface;



    RecommendationsProcessor reloader(db, fanficsInterface,
                                      fandomInterface,
                                      authorInterface,
                                      recsInterface);

    connect(&reloader, &RecommendationsProcessor::resetEditorText, this,    &ServitorWindow::OnResetTextEditor);
    connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &ServitorWindow::OnProgressBarRequested);
    connect(&reloader, &RecommendationsProcessor::updateCounter, this,      &ServitorWindow::OnUpdatedProgressValue);
    connect(&reloader, &RecommendationsProcessor::updateInfo, this,         &ServitorWindow::OnNewProgressString);


    reloader.SetStagedAuthors(authors);
    reloader.ReloadRecommendationsList(ECacheMode::use_only_cache);
    authorInterface->AssignAuthorNamesForWebIDsInFanficTable();
}

void ServitorWindow::on_pbGetNewFavourites_clicked()
{
    if(!env.ResumeUnfinishedTasks())
        env.LoadAllLinkedAuthors(ECacheMode::use_cache);
}

void ServitorWindow::on_pbReprocessCacheLinked_clicked()
{
    env.LoadAllLinkedAuthorsMultiFromCache();
}

void ServitorWindow::on_pbPCRescue_clicked()
{
    QString path = "PageCache.sqlite";
    QSqlDatabase pcdb = QSqlDatabase::addDatabase("QSQLITE", "PageCache");
    pcdb.setDatabaseName(path);
    pcdb.open();


    path = "PageCache_export.sqlite";
    QSqlDatabase pcExDb = QSqlDatabase::addDatabase("QSQLITE", "PageCache_Export");
    pcExDb.setDatabaseName(path);
    pcExDb.open();


    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> pageCacheExportInterface (new database::SqliteInterface());

    pcdb = pageCacheInterface->InitDatabase("PageCache");
    qDebug() << "Db open: " << pcdb.isOpen();
    QSqlQuery testQuery("select * from pagecache where url = 'https://www.fanfiction.net/u/1000039'", pcdb);
    bool readable = testQuery.next();
    qDebug() << "Readable: " << readable;
    qDebug() << "Data: " << testQuery.record();

    pcExDb = pageCacheExportInterface->InitDatabase("PageCache_Export");

    pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "PageCache");
    pageCacheExportInterface->ReadDbFile("dbcode/pagecacheinit.sql", "PageCache_Export");


    int counter = 0;

    QString insert = "INSERT INTO PAGECACHE(URL, GENERATION_DATE, CONTENT,  PAGE_TYPE, COMPRESSED) "
                     "VALUES(:URL, :GENERATION_DATE, :CONTENT, :PAGE_TYPE, :COMPRESSED)";
    database::Transaction transactionMain(env.interfaces.db->GetDatabase());
    //database::Transaction transactionRead(pcdb);
    QSqlQuery exportQ(pcExDb);
    exportQ.prepare(insert);
    auto authors = env.interfaces.authors->GetAllAuthorsUrls("ffn");
    std::sort(authors.begin(), authors.end());
    qDebug() << "finished reading author urls";
    QSqlQuery readQuery(pcdb);
    readQuery.prepare("select * from pagecache where url = :url");

    database::Transaction transaction(pcExDb);

    for(auto author : authors)
    {
        //qDebug() << "Attempting to read url: " << author;
        readQuery.bindValue(":url", author);
        readQuery.exec();
        bool result = readQuery.next();
        if(!result || readQuery.value("url").toString().isEmpty())
        {
            qDebug() << "Attempting to read url: " << author;
            qDebug() << readQuery.lastError().text();
            continue;
        }
        if(counter%1000 == 0)
        {
            qDebug() << "committing: " << counter;
            transaction.finalize();
            transaction.start();
        }

        //qDebug() << "writing record: " << readQuery.value("url").toString();

        counter++;
        exportQ.bindValue(":URL", readQuery.value("url").toString());
        exportQ.bindValue(":GENERATION_DATE", readQuery.value("GENERATION_DATE").toDateTime());
        exportQ.bindValue(":CONTENT", readQuery.value("CONTENT").toByteArray());
        exportQ.bindValue(":COMPRESSED", readQuery.value("COMPRESSED").toInt());
        exportQ.bindValue(":PAGE_TYPE", readQuery.value("PAGE_TYPE").toString());
        exportQ.exec();
        if(exportQ.lastError().isValid())
            qDebug() << "Error writing record: " << exportQ.lastError().text();
    }


}

void ServitorWindow::on_pbSlashCalc_clicked()
{
    auto db = QSqlDatabase::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    authorInterface->portableDBInterface = dbInterface;

    auto authors = authorInterface->GetAllAuthors("ffn");


    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;
    fandomInterface->portableDBInterface = dbInterface;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->portableDBInterface = dbInterface;
    recsInterface->authorInterface = authorInterface;

    SlashProcessor slash(db,fanficsInterface, fandomInterface, authorInterface, recsInterface, dbInterface);
    slash.DoFullCycle(db, 2);

}

void ServitorWindow::on_pbFindSlashSummary_clicked()
{
    CommonRegex rx;
    rx.Init();
    auto result = rx.ContainsSlash("A year after his mother's death, Ichigo finds himself in a world that he knows doesn't exist and met four spirits. Deciding to know what they truly are, he goes to his father who takes him to a shop. Take a different road IchiHime fans. Dual Wield Zanpakuto! Resurreccion! Quincy powers! OOC. New chapters every week or two. HIATUS",
                                   "[Ichigo K., Yoruichi S.] Rukia K., T. Harribel",
                                   "Bleach");
}
