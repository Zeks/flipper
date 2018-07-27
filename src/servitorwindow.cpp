#include "include/servitorwindow.h"
#include "include/Interfaces/genres.h"
#include "include/Interfaces/fanfics.h"
#include "include/url_utils.h"
#include <QTextCodec>
#include <QSettings>
#include "ui_servitorwindow.h"
#include "include/parsers/ffn/ficparser.h"
#include "pagegetter.h"


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
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    genres->db = QSqlDatabase::database();
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::Genres());
    fanfics->db = QSqlDatabase::database();

    QSettings settings("servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    bool alreadyQueued = settings.value("Settings/genrequeued", false).toBool();
    if(!alreadyQueued)
    {
        genres->QueueFicsForGenreDetection(25, 15);
        settings.setValue("Settings/genrequeued", true);
        settings.sync();
    }

    for(auto fic : genres->GetGenreDataForQueuedFics())
    {
        converter.ProcessGenreResult(fic);
    }
}
