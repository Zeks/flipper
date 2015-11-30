#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QRegExp>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <algorithm>




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(
                &manager, SIGNAL (finished(QNetworkReply*)),
                this, SLOT (OnNetworkReply(QNetworkReply*)));
    browserMenu.addAction("Hide fanfic", this, SLOT(OnHideFanfic()));
    browserMenu.addAction("Smut fanfic", this, SLOT(OnSmutFanfic()));
    browserMenu.addAction("Unknown Fandom", this, SLOT(OnUnknownFandom()));
    ui->edtResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->edtResults, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnShowContextMenu(QPoint)));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::RequestPage(QString page)
{
    if(page.isEmpty())
        page = ui->leUrl->text();
    QString origin = ui->leStartFromPage->text();

    QString pageCount = ui->lePageLimit->text();
    if(pageCount.isEmpty() || pageCount.toInt() < 0 || pageCount.toInt() > 1000)
    {
        if(QMessageBox::question(nullptr, "Warning!","Page count is questionable. Really use it?") != QMessageBox::Yes)
            return;
    }
    manager.get(QNetworkRequest(QUrl(page)));
}

void MainWindow::ProcessPage(QString str)
{
    Section section;
    int currentPosition = 0;
    int counter = 0;
    QList<Section> sections;
    while(true)
    {

        counter++;
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        QString temp = str.mid(section.start, section.end - section.start);
        //ui->edtDebug->insertPlainText(temp);
        currentPosition = section.start;
        if(ui->leCategory->text().isEmpty())
            section.fandom = GetFandom(str).replace("\n", "");
        else
            section.fandom = ui->leCategory->text();
        GetUrl(section, currentPosition, str);
        //qDebug() << section.url;
        //ui->edtDebug->insertPlainText(section.url + "\n");
        GetTitle(section, currentPosition, str);
        //qDebug() << section.title;
        //ui->edtDebug->insertPlainText(section.title + "\n");
        GetAuthor(section, currentPosition, str);
        //qDebug() << section.author;
        //ui->edtDebug->insertPlainText(section.author + "\n");
        GetSummary(section, currentPosition, str);
        //qDebug() << section.summary;
        //ui->edtDebug->insertPlainText(section.summary + "\n");
        GetWordCount(section, currentPosition, str);
        //qDebug() << section.wordCount;
        //ui->edtDebug->insertPlainText(QString::number(section.wordCount) + "\n");
        GetGenre(section, currentPosition, str);
        //qDebug() << section.genre;
        ///ui->edtDebug->insertPlainText(section.genre + "\n");
        GetUpdatedDate(section, currentPosition, str);
        //qDebug() << section.updated;
        //ui->edtDebug->insertPlainText(section.updated.toString() + "\n");
        GetPublishedDate(section, currentPosition, str);
        //qDebug() << section.published;
        //ui->edtDebug->insertPlainText(section.published.toString() + "\n");
        //ui->edtDebug->insertPlainText("\n");
        if(section.isValid)
        {
            sections.append(section);
        }
        qDebug() << "page: " << processedCount << " item: " << counter;
    }
    for(auto section : sections)
    {
        LoadIntoDB(section);
        ui->edtDebug->insertPlainText("Written:" + section.title + " by " + section.author + "\n");
    }
    if(sections.size() > 0)
        GetNext(sections.last(), currentPosition, str);
    currentPosition = 999;
    ui->edtDebug->insertPlainText("\n Next page \n");
    if(processedCount < ui->lePageLimit->text().toInt())
        timerId = startTimer(1000);
    //xicon-section-arrow'></span>
}

QString MainWindow::GetFandom(QString text)
{
    QRegExp rxStart(QRegExp::escape("xicon-section-arrow'></span>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text, 0);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    return text.mid(indexStart + 28,indexEnd - (indexStart + 28));
}

void MainWindow::GetAuthor(Section & section, int& startfrom, QString text)
{
    QRegExp rxBy("by\\s<");
    QRegExp rxStart(">");
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexBy = rxBy.indexIn(text, startfrom);
    int indexStart = rxStart.indexIn(text, indexBy + 3);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.author = text.mid(indexStart + 1,indexEnd - (indexStart + 1));

}

void MainWindow::GetTitle(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.title = text.mid(indexStart + 1,indexEnd - (indexStart + 1));
}

void MainWindow::GetGenre(Section & section, int& , QString text)
{
    //need to check if genre section even exists
    QString toCheck(text.mid(section.summaryEnd, section.wordCountStart - section.summaryEnd));
    int count = std::count_if(toCheck.begin(), toCheck.end(), [](QString val){return val == "-";});
    if(count == 3)
        return ;
    QRegExp rxStart(QRegExp::escape("-"));

    int indexStart = rxStart.indexIn(text, section.summaryEnd);
    indexStart = rxStart.indexIn(text, indexStart + 1);
    indexStart = rxStart.indexIn(text, indexStart + 1);
    int indexEnd = rxStart.indexIn(text, indexStart + 1);
    section.genre = text.mid(indexStart,indexEnd - indexStart).trimmed();
    section.genre = section.genre .replace("/", " ");
    section.genre = section.genre .replace("-", "").trimmed();
}

void MainWindow::GetSummary(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("padtop'>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;


}

void MainWindow::GetWordCount(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart("-\\sWords:");
    QRegExp rxEnd("\\s-");
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    QString words = text.mid(indexStart + 9,indexEnd - (indexStart + 9));
    words=words.replace(" ", "");
    words=words.replace(",", "");
    section.wordCount = words.toInt();
    section.wordCountStart = indexStart;
    if(startfrom <=  indexEnd)
        startfrom = indexEnd;


}

void MainWindow::GetPublishedDate(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("<"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);
    //qDebug() << "PublishText: " << text.mid(indexStart+1, indexEnd - (indexStart +1));
    QDateTime temp = ConvertToDate(text.mid(indexStart + 1,indexEnd - (indexStart + 1)));
    if(!temp.isValid())
        qDebug() << text.mid(indexStart + 1,indexEnd - (indexStart + 1));
    section.published = ConvertToDate(text.mid(indexStart + 1,indexEnd - (indexStart + 1)));
    if(startfrom <=  indexEnd)
        startfrom = indexEnd;
}

void MainWindow::GetUpdatedDate(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("<"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);
    //qDebug() << "UpdateText: " << text.mid(indexStart+1, indexEnd - (indexStart +1));
    section.updated = ConvertToDate(text.mid(indexStart + 1,indexEnd - (indexStart + 1)));
    startfrom = indexEnd+12;

}

void MainWindow::GetUrl(Section & section, int& startfrom, QString text)
{
    // looking for first href
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\"><img"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.url = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    startfrom = indexEnd+2;
}

void MainWindow::GetNext(Section & section, int &startfrom, QString text)
{
    QRegExp rxStart("Last</a>\\s<a\\shref='");
    QRegExp rxEnd(QRegExp::escape("'>Next"));

    int indexStart = rxStart.indexIn(text,section.start);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    //indexStart = rxStart.indexIn(text,indexStart+1);

    int indexEnd = rxEnd.indexIn(text, indexStart);
    nextUrl = CreateURL(text.mid(indexStart+19, indexEnd - (indexStart+19)));
    indexEnd = rxEnd.indexIn(text, indexStart);
}

QDateTime MainWindow::ConvertToDate(QString text)
{
    int count = std::count_if(text.begin(), text.end(), [](QString val){return val == "/";});
    QDateTime result;

    if(count == 0)
    {
        //hours
        result = QDateTime::currentDateTime().addSecs(-1*60*60*text.replace(QRegExp("[\\sh]"), "").toInt());
    }
    if(count == 1)
    {
        //with month
        result = QDateTime::fromString(text.trimmed() + QString("/") + QString::number(QDateTime::currentDateTime().date().year()) , "M/d/yyyy");
    }
    if(count == 2)
    {
        result = QDateTime::fromString(text.trimmed() , "M/d/yyyy");
    }
    return result;
}

Section MainWindow::GetSection(QString text, int start)
{
    Section section;
    QRegExp rxStart("<div\\sclass=\'z-list\\szhover\\szpointer\\s\'");
    int index = rxStart.indexIn(text, start);
    if(index != -1)
    {
        section.isValid = true;
        section.start = index;
        int end = rxStart.indexIn(text, index+1);
        if(end == -1)
            end = index + 2000;
        section.end = end;
    }
    return section;
}

void MainWindow::SkipPages(int)
{
    //intentionally blank
}

QString MainWindow::CreateURL(QString str)
{
    return "https://www.fanfiction.net/" + str;
}

void MainWindow::LoadData()
{
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(path);
    db.open();
    qDebug() << db.lastError();

    QString queryString = "select rowid, f.* from fanfics f where wordcount > :wordcount ";
    if(!ui->leContainsGenre->text().isEmpty())
        for(auto genre : ui->leContainsGenre->text().split(" "))
            queryString += QString(" AND genres like '%%1%' ").arg(genre);

    if(!ui->leNotContainsGenre->text().isEmpty())
        for(auto genre : ui->leNotContainsGenre->text().split(" "))
            queryString += QString(" AND genres not like '%%1%' ").arg(genre);

    if(ui->chkSmut->isChecked())
        queryString+=" and smut = 1 ";
    else if(ui->chkUnknownFandoms->isChecked())
        queryString+=" and unknown_fandom = 1 ";
    else
    {
        queryString+=" and smut = 0 ";
        queryString+=" and unknown_fandom = 0 ";
        queryString+=" and  hidden = 0 ";
    }
    if(!ui->leFandom->text().isEmpty())
        queryString+=QString(" and  fandom = '%1'").arg(ui->leFandom->text());
    queryString+=" ORDER BY WORDCOUNT DESC ";



    QSqlQuery q(db);
    q.prepare(queryString);
    q.bindValue(":wordcount", ui->leMinWordCount->text().toInt());
    q.exec();
    qDebug() << q.lastError();
    ui->edtResults->setOpenExternalLinks(true);
    ui->edtResults->clear();
    ui->edtResults->setFont(QFont("Verdana", 20));
    while(q.next())
    {
        QString toInsert = "<a href=\"" + CreateURL((q.value("URL").toString()) + "\"> %1 </a>");
        toInsert= toInsert.arg(q.value("TITLE").toString());
        ui->edtResults->insertHtml(toInsert);
        ui->edtResults->append("<span>" + QString("By: ") + q.value("AUTHOR").toString() + "</span>");
        ui->edtResults->append("<span>" + QString(" ") + q.value("SUMMARY").toString() + "</span>");
        ui->edtResults->append("<span>" + QString("<font color=\"green\">Genre: ") + q.value("GENRES").toString() + "</font></span>");
        ui->edtResults->append("<span>" + QString("<font color=\"green\">Words: ") + q.value("WORDCOUNT").toString() + "</font></span>");
        ui->edtResults->append("<span>" + QString("ID: ") +  QString::number(q.value("rowid").toInt()) +  "</font></span>");

        ui->edtResults->insertPlainText("\n");
        ui->edtResults->insertPlainText("\n");

    }


}

void MainWindow::HideFanfic(int id)
{
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::database(path);//not dbConnection
    QSqlQuery q(db);
    q.prepare(QString("update fanfics set hidden = 1 where rowid = %1").arg(id));
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();


}

void MainWindow::SmutFanfic(int id)
{
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::database(path);//not dbConnection
    QSqlQuery q(db);
    q.prepare(QString("update fanfics set smut = 1 where rowid = %1").arg(id));
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
}

void MainWindow::UnknownFandom(int id)
{
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::database(path);//not dbConnection
    QSqlQuery q(db);
    q.prepare(QString("update fanfics set unknown_fandom = 1 where rowid = %1").arg(id));
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
}

void MainWindow::timerEvent(QTimerEvent *)
{
    if(processedCount < ui->lePageLimit->text().toInt()
            && !nextUrl.isEmpty())
    {
        killTimer(timerId);
        timerId = -1;
        qDebug() << "Getting: " <<  nextUrl;
        manager.get(QNetworkRequest(QUrl(nextUrl)));

    }
}

void MainWindow::LoadIntoDB(Section & section)
{
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::database(path);//not dbConnection

    bool isUpdate = false;
    QString getKeyQuery = "Select count(*) from FANFICS where AUTHOR = '%1' and TITLE = '%2'";
    getKeyQuery = getKeyQuery.arg(QString(section.author).replace("'","''")).arg(QString(section.title).replace("'","''"));
    QSqlQuery keyQ(getKeyQuery, db);
    keyQ.next();
    //qDebug() << keyQ.value(0).toInt();
    if(keyQ.value(0).toInt() > 0)
        isUpdate = true;


    if(!isUpdate)
    {

        qDebug() << "Inserting: " << section.author << " " << section.title;
        QString query = "INSERT INTO FANFICS (FANDOM, AUTHOR, TITLE,WORDCOUNT, SUMMARY, GENRES, PUBLISHED, UPDATED, URL) "
                        "VALUES (  :fandom, :author, :title, :wordcount, :summary, :genres, :published, :updated, :url)";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author);
        q.bindValue(":title",section.title);
        q.bindValue(":wordcount",section.wordCount);
        q.bindValue(":summary",section.summary);
        q.bindValue(":genres",section.genre);
        q.bindValue(":published",section.published);
        q.bindValue(":updated",section.updated);
        q.bindValue(":url",section.url);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << "failed to insert: " << section.author << " " << section.title;
            qDebug() << q.lastError();
        }

    }

    if(isUpdate)
    {
        qDebug() << "Updating: " << section.author << " " << section.title;
        QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, summary = :summary, genres= :genres, published = :published, updated = :updated, url = :url "
                        " where author = :author and title = :title";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author);
        q.bindValue(":title",section.title);
        q.bindValue(":wordcount",section.wordCount);
        q.bindValue(":summary",section.summary);
        q.bindValue(":genres",section.genre);
        q.bindValue(":published",section.published);
        q.bindValue(":updated",section.updated);
        q.bindValue(":url",section.url);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << "failed to update: " << section.author << " " << section.title;
            qDebug() << q.lastError();
        }
    }
}

void MainWindow::OnNetworkReply(QNetworkReply * reply)
{
    ++processedCount;
    QByteArray data=reply->readAll();
    qDebug() << reply->error();
    reply->deleteLater();
    QString str(data);
    ProcessPage(str);
}

void MainWindow::on_pbCrawl_clicked()
{

    pageCounter = 0;
    ui->edtDebug->clear();
    processedCount = 0;
    nextUrl = QString();
    SkipPages(ui->leStartFromPage->text().toInt()-1);
    RequestPage(ui->leUrl->text());
}

void MainWindow::OnHideFanfic()
{
    bool ok = false;
    int value = ui->edtResults->textCursor().selectedText().trimmed().toInt(&ok);
    if(ok)
    {
        auto cursorPosition = ui->edtResults->textCursor().position();
        HideFanfic(value);
        LoadData();
        auto cursor = ui->edtResults->textCursor();
        cursor.setPosition(cursorPosition);
        ui->edtResults->setTextCursor(cursor);
    }

}

void MainWindow::OnSmutFanfic()
{
    bool ok = false;
    int value = ui->edtResults->textCursor().selectedText().trimmed().toInt(&ok);
    if(ok)
    {
        auto cursorPosition = ui->edtResults->textCursor().position();
        SmutFanfic(value);
        LoadData();
        auto cursor = ui->edtResults->textCursor();
        cursor.setPosition(cursorPosition);
        ui->edtResults->setTextCursor(cursor);
    }

}

void MainWindow::OnUnknownFandom()
{
    bool ok = false;
    int value = ui->edtResults->textCursor().selectedText().trimmed().toInt(&ok);
    if(ok)
    {
        auto cursorPosition = ui->edtResults->textCursor().position();
        UnknownFandom(value);
        LoadData();
        auto cursor = ui->edtResults->textCursor();
        cursor.setPosition(cursorPosition);
        ui->edtResults->setTextCursor(cursor);
    }
}

void MainWindow::OnShowContextMenu(QPoint p)
{
   browserMenu.popup(this->mapToGlobal(p));
}

void MainWindow::on_pbLoadDatabase_clicked()
{
    LoadData();
}

void MainWindow::on_chkSmut_toggled(bool checked)
{
    LoadData();
}

void MainWindow::on_chkUnknownFandoms_toggled(bool checked)
{
    LoadData();
}
