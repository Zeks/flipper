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
#include <QTextCodec>
#include <QSettings>
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
    interfaces::GenreConverter converter;

    QVector<int> ficIds;
    auto db = QSqlDatabase::database();
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    genres->db = db;
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanfics->db = db;

    QSettings settings("settings_servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    bool alreadyQueued = settings.value("Settings/genrequeued", false).toBool();
    if(!alreadyQueued)
    {
        database::Transaction transaction(db);
        genres->QueueFicsForGenreDetection(25, 15, 0);
        settings.setValue("Settings/genrequeued", true);
        settings.sync();
        transaction.finalize();
    }
    database::Transaction transaction(db);
    auto genreData = genres->GetGenreDataForQueuedFics();
    for(auto& fic : genreData)
    {
        converter.ProcessGenreResult(fic);
    }
    if(!genres->WriteDetectedGenres(genreData))
        transaction.cancel();

    transaction.finalize();
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
