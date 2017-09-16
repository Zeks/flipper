#include "include/servitorwindow.h"
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
    QString url = ui->leFicUrl->text();
    auto page = pager.GetPage(url, ECacheMode::use_cache);
    parser.SetRewriteAuthorNames(false);
    parser.ProcessPage(url, page.content);
    parser.WriteProcessed();
}
