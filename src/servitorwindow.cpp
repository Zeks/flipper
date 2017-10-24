#include "include/servitorwindow.h"
#include "include/url_utils.h"
#include "ui_servitorwindow.h"
#include "ficparser.h"
#include "pagegetter.h"
#include <QTextCodec>

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
    PageManager pager;
    FicParser parser;
    QHash<QString, int> fandoms;
    auto result = database::GetAllFandoms(fandoms);
    if(!result)
        return;
    QString url = ui->leFicUrl->text();
    auto page = pager.GetPage(url, ECacheMode::use_cache);
    parser.SetRewriteAuthorNames(false);
    parser.ProcessPage(url, page.content);
    parser.WriteProcessed(fandoms);
}

void ServitorWindow::on_pbReprocessFics_clicked()
{
    PageManager pager;
    FicParser parser;
    QHash<QString, int> fandoms;
    auto result = database::GetAllFandoms(fandoms);
    if(!result)
        return;
    QSqlDatabase db = QSqlDatabase::database();
    //db.transaction();
    database::ReprocessFics(" where fandom1 like '% CROSSOVER' and alive = 1 order by id asc", "ffn", [this,&pager, &parser, &fandoms](int ficId){
        //todo, get web_id from fic_id
        QString url = url_utils::GetUrlFromWebId(webId, "ffn");
        auto page = pager.GetPage(url, ECacheMode::use_only_cache);
        parser.SetRewriteAuthorNames(false);
        auto fic = parser.ProcessPage(url, page.content);
        if(fic.isValid)
            parser.WriteProcessed(fandoms);
    });
    //db.commit();


}

void ServitorWindow::on_pushButton_clicked()
{
    database::EnsureFandomsNormalized();
}
