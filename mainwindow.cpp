#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QRegExp>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QPair>
#include <QStringListModel>
#include <algorithm>




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("ffnet sane search engine");

    connect(
                &manager, SIGNAL (finished(QNetworkReply*)),
                this, SLOT (OnNetworkReply(QNetworkReply*)));
    browserMenu.addAction("Hide fanfic", this, SLOT(OnHideFanfic()));
    browserMenu.addAction("Smut fanfic", this, SLOT(OnSmutFanfic()));
    browserMenu.addAction("Unknown Fandom", this, SLOT(OnUnknownFandom()));
    ui->edtResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->edtResults, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnShowContextMenu(QPoint)));
    connect(&fandomManager, &QNetworkAccessManager::finished, this, &MainWindow::OnFandomReply);
    connect(&crossoverManager, &QNetworkAccessManager::finished, this, &MainWindow::OnCrossoverReply);
    connect(ui->cbSectionTypes, SIGNAL(currentTextChanged(QString)), this, SLOT(OnSectionChanged(QString)));
    nameOfFandomSectionToLink["Normal/Anime/Manga"] = QString::fromLocal8Bit("https://www.fanfiction.net/anime/");
    nameOfFandomSectionToLink["Normal/Book"] = QString::fromLocal8Bit("https://www.fanfiction.net/book/");
    nameOfCrossoverSectionToLink["Crossover/Anime/Manga"] = QString::fromLocal8Bit("https://www.fanfiction.net/crossovers/anime/");
    nameOfCrossoverSectionToLink["Crossover/Book"] = QString::fromLocal8Bit("https://www.fanfiction.net/crossovers/book/");

    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Init()
{
    UpdateFandomList();
    UpdateCrossoverList();
    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));
}

void MainWindow::RequestPage(QString page)
{
    //if(page.isEmpty())
//        page = ui->leUrl->text();
    //    QString origin = ui->leStartFromPage->text();

    //    QString pageCount = ui->lePageLimit->text();
    //    if(pageCount.isEmpty() || pageCount.toInt() < 0 || pageCount.toInt() > 1000)
    //    {
    //        if(QMessageBox::question(nullptr, "Warning!","Page count is questionable. Really use it?") != QMessageBox::Yes)
    //            return;
    //    }
    QString toInsert = "<a href=\"" + page + "\"> %1 </a>";
    toInsert= toInsert.arg(page);
    ui->edtResults->append("<span>Processing url: </span>");
    ui->edtResults->insertHtml(toInsert);
    manager.get(QNetworkRequest(QUrl(page)));
}

void MainWindow::ProcessPage(QString str)
{
    Section section;
    int currentPosition = 0;
    int counter = 0;
    QList<Section> sections;
    bool abort = false;
    while(true)
    {

        counter++;
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        QString temp = str.mid(section.start, section.end - section.start);
        //ui->edtDebug->insertPlainText(temp);
        currentPosition = section.start;

        section.fandom = ui->cbCrossovers->currentText().isEmpty() ? ui->cbNormals->currentText() : ui->cbCrossovers->currentText() + " CROSSOVER";
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
        if(section.fandom.contains("CROSSOVER"))
            GetCrossoverFandomList(section, currentPosition, str);

        GetWordCount(section, currentPosition, str);
        //qDebug() << section.wordCount;
        //ui->edtDebug->insertPlainText(QString::number(section.wordCount) + "\n");

        GetGenre(section, currentPosition, str);
        //qDebug() << section.genre;
        ///ui->edtDebug->insertPlainText(section.genre + "\n");
        GetUpdatedDate(section, currentPosition, str);


        if(section.updated.toMSecsSinceEpoch() < lastUpdated.toMSecsSinceEpoch())
            abort = true;
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
    if(sections.size() == 0)
    {
        ui->edtResults->insertHtml("<span> No data found on the page.<br></span>");
        ui->edtResults->insertHtml("<span> \nFinished loading data <br></span>");
        return;
    }


    for(auto section : sections)
    {
        LoadIntoDB(section);
        ui->edtResults->insertHtml("<span> Written:" + section.title + " by " + section.author + "\n <br></span>");
    }
    if(abort)
    {
        ui->edtResults->insertHtml("<span> Already have updates past that point, aborting<br></span>");
        return;
    }
    if(sections.size() > 0)
        GetNext(sections.last(), currentPosition, str);
    currentPosition = 999;
    ui->edtResults->insertHtml("<span> \n Next page \n <br></span>");

    if(!nextUrl.isEmpty())
        timerId = startTimer(1000);

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
//    QString toCheck(text.mid(section.summaryEnd, section.wordCountStart - section.summaryEnd));
//    int count = std::count_if(toCheck.begin(), toCheck.end(), [](QString val){return val == "-";});
//    if(count == 3)
//        return ;
    QRegExp rxRated(QRegExp::escape("English"));
    QRegExp rxChapters(QRegExp::escape("Chapters"));

    int indexRated= rxRated.indexIn(text, section.summaryEnd);
    int indexChapters = rxChapters.indexIn(text, indexRated + 1);

    auto ref = text.midRef(indexRated + rxRated.pattern().length(), indexChapters - (indexRated + rxRated.pattern().length()));
    int count = std::count_if(ref.begin(),ref.end(), [](QChar value){ return value == '-';});
    if(count < 2)
            return ;

    QString genre = ref.mid(2, ref.length() - 2).toString();
    genre=genre.replace("/", " ").replace("-", "").trimmed();
    section.genre = genre;

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

void MainWindow::GetCrossoverFandomList(Section & section, int &startfrom, QString text)
{
    QRegExp rxStart("Crossover\\s-\\s");
    QRegExp rxEnd("\\s-\\sRated:");

    int indexStart = rxStart.indexIn(text, startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    section.fandom = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed().replace("&", " ") + QString(" CROSSOVER");
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
    if(!section.updated.isValid())
        qDebug() << text.mid(indexStart + 1,indexEnd - (indexStart + 1));
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
    QRegExp rxEnd(QRegExp::escape("Next &#187"));
    int indexEnd = rxEnd.indexIn(text, startfrom);
    int posHref = indexEnd - 400 + text.midRef(indexEnd - 400,400).lastIndexOf("href='");
    //int indexStart = rxStart.indexIn(text,section.start);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    nextUrl = CreateURL(text.mid(posHref+6, indexEnd - (posHref+6)));
    if(!nextUrl.contains("&p="))
        nextUrl = "";
    indexEnd = rxEnd.indexIn(text, startfrom);
}

QDateTime MainWindow::ConvertToDate(QString text)
{
    int count = std::count_if(text.begin(), text.end(), [](QString val){return val == "/";});
    QDateTime result;

    if(count == 0)
    {
        //hours
        if(text.contains("d"))
            result = QDateTime::currentDateTime().addDays(-1*text.replace(QRegExp("[\\sd]"), "").toInt());
        if(text.contains("h"))
            result = QDateTime::currentDateTime().addSecs(-1*60*60*text.replace(QRegExp("[\\sh]"), "").toInt());
        if(text.contains("m"))
            result = QDateTime::currentDateTime().addSecs(-1*60*text.replace(QRegExp("[\\sm]"), "").toInt());
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
    if(!ui->cbNormals->currentText().isEmpty())
        queryString+=QString(" and  fandom = like '%%1%'").arg(ui->cbNormals->currentText());
    else if(!ui->cbCrossovers->currentText().isEmpty())
        queryString+=QString(" and  fandom like '%%1%'").arg(ui->cbCrossovers->currentText());
    queryString+=" ORDER BY WORDCOUNT DESC ";



    QSqlQuery q(db);
    q.prepare(queryString);
    q.bindValue(":wordcount", ui->leMinWordCount->text().toInt());
    q.exec();
    qDebug() << q.lastQuery();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
    ui->edtResults->setOpenExternalLinks(true);
    ui->edtResults->clear();
    ui->edtResults->setFont(QFont("Verdana", 20));
    int counter = 0;
    while(q.next())
    {
        counter++;
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
        if(counter%100 == 0)
            qDebug() << "tick " << counter/100;
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
    if(!nextUrl.isEmpty())
    {
        killTimer(timerId);
        timerId = -1;
        qDebug() << "Getting: " <<  nextUrl;
        QString toInsert = "<a href=\"" + nextUrl + "\"> %1 </a>";
        toInsert= toInsert.arg(nextUrl);
        ui->edtResults->append("<span> Processing url: </span>");
        ui->edtResults->insertHtml(toInsert);
        manager.get(QNetworkRequest(QUrl(nextUrl)));

    }
}

void MainWindow::UpdateFandomList()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    names.clear();

    for(auto section : nameOfFandomSectionToLink.keys())
    {
        currentProcessedSetion = section;

        fandomManager.get(QNetworkRequest(QUrl(nameOfFandomSectionToLink[section])));
        managerEventLoop.exec();
    }
    for(auto value : nameOfFandomSectionToLink.keys())
    {

        QString qs = QString("Select fandom from FANdoms where section = '%1'").arg(value.replace("'","''"));
        QSqlQuery q(qs, db);
        QHash<QString, QString> knownValues;
        while(q.next())
        {
            knownValues[q.value(0).toString()] = q.value(0).toString();
        }
        qDebug() << q.lastError();

        for(auto fandom : names[value])
        {
            if(!knownValues.contains(fandom.name))
            {
                QString insert = "INSERT INTO FANDOMS (FANDOM, URL, SECTION) VALUES (:FANDOM, :URL, :SECTION)";
                QSqlQuery q(db);
                q.prepare(insert);
                q.bindValue(":FANDOM",fandom.name.replace("'","''"));
                q.bindValue(":URL",fandom.url.replace("'","''"));
                q.bindValue(":SECTION",value.replace("'","''"));
                q.exec();
                if(q.lastError().isValid())
                    qDebug() << q.lastError();
            }
        }

    }
}


void MainWindow::UpdateCrossoverList()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    names.clear();

    for(auto section : nameOfCrossoverSectionToLink.keys())
    {
        currentProcessedSetion = section;

        crossoverManager.get(QNetworkRequest(QUrl(nameOfCrossoverSectionToLink[section])));
        managerEventLoop.exec();
    }
    for(auto value : nameOfCrossoverSectionToLink.keys())
    {

        QString qs = QString("Select fandom from FANdoms where section = '%1'").arg(value.replace("'","''"));
        QSqlQuery q(qs, db);
        QHash<QString, QString> knownValues;
        while(q.next())
        {
            knownValues[q.value(0).toString()] = q.value(0).toString();
        }
        qDebug() << q.lastError();

        for(auto fandom : names[value])
        {
            if(!knownValues.contains(fandom.name))
            {
                QString insert = "INSERT INTO FANDOMS (FANDOM, URL, SECTION) VALUES (:FANDOM, :URL, :SECTION)";
                QSqlQuery q(db);
                q.prepare(insert);
                q.bindValue(":FANDOM",fandom.name.replace("'","''" ));
                q.bindValue(":URL",fandom.url.replace("'","''"));
                q.bindValue(":SECTION",value.replace("'","''"));
                q.exec();
                if(q.lastError().isValid())
                    qDebug() << q.lastError();
            }
        }

    }
}

QStringList MainWindow::GetFandomListFromDB()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select fandom from fandoms where section like '%Normal%' and section like '%%1%'").arg(ui->cbSectionTypes->currentText());
    QSqlQuery q(qs, db);
    QStringList result;
    result.append("");
    while(q.next())
    {
        result.append(q.value(0).toString());
    }
    return result;
}

QStringList MainWindow::GetCrossoverListFromDB()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select fandom from fandoms where section like '%Crossover%' and section like '%%1%'").arg(ui->cbSectionTypes->currentText());
    QSqlQuery q(qs, db);
    QStringList result;
    result.append("");
    while(q.next())
    {
        result.append(q.value(0).toString());
    }
    return result;
}

QString MainWindow::GetCrossoverUrl(QString)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select url from fandoms where section like '%Crossover%' and section like '%%1%' and fandom = '%2'").arg(ui->cbSectionTypes->currentText()).arg(ui->cbCrossovers->currentText());
    QSqlQuery q(qs, db);
    q.next();
    QString rebindName = q.value(0).toString();
    QStringList temp = rebindName.split("/");

    rebindName = "/" + temp.at(2) + "-Crossovers" + "/" + temp.at(3);
    QString result =  "https://www.fanfiction.net" + rebindName + "/0/?&srt=1&lan=1&r=10&len=100";

    qDebug() << result;
    return result;
}

QString MainWindow::GetNormalUrl(QString)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select url from fandoms where section like '%Normal%' and section like '%%1%' and fandom = '%2'").arg(ui->cbSectionTypes->currentText()).arg(ui->cbNormals->currentText());
    //qDebug() << qs;
    QSqlQuery q(qs, db);
    q.next();
    QString result = "https://www.fanfiction.net" + q.value(0).toString() + "/?&srt=1&lan=1&r=10&len=100";
    qDebug() << result;
    return result;
}

QDateTime MainWindow::GetMaxUpdateDateForSection(QString section)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select max(updated) as updated from fanfics where fandom like '%1'").arg(section);
    QSqlQuery q(qs, db);
    q.next();
    QDateTime result = q.value(0).toDateTime();
    qDebug() << result;
    return result;
}

void MainWindow::LoadIntoDB(Section & section)
{

    QSqlDatabase db = QSqlDatabase::database(dbName);

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

        qDebug() << "Inserting: " << section.author << " " << section.title << " " << section.fandom << " " << section.genre;
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

void MainWindow::OnFandomReply(QNetworkReply * reply)
{

    QByteArray data=reply->readAll();
    qDebug() << reply->error();
    QString str(data);
    reply->deleteLater();
    // getting to the start of fandom section
    QRegExp rxStartFandoms("list_output");
    int indexStart = rxStartFandoms.indexIn(str);
    if(indexStart == -1)
    {
        QMessageBox::warning(0, "warning!", "failed to find the start of fandom section");
        return;
    }

    QRegExp rxEndFandoms("</TABLE>");
    int indexEnd= rxEndFandoms.indexIn(str);
    if(indexEnd == -1)
    {
        QMessageBox::warning(0, "warning!", "failed to find the end of fandom section");
        return;
    }
    int counter = 0;
    while(true)
    {
        QRegExp rxStartLink("href=\"");
        QRegExp rxEndLink("/\"");

        int linkStart = rxStartLink.indexIn(str, indexStart);
        if(linkStart == -1)
            break;
        int linkEnd= rxEndLink.indexIn(str, linkStart);
        if(linkStart == -1 || linkEnd == -1)
        {
            QMessageBox::warning(0, "warning!", "failed to fetch link at: ", str.mid(linkStart, str.size() - linkStart));
        }
        QString link = str.mid(linkStart + rxStartLink.pattern().length(),
                               linkEnd - (linkStart + rxStartLink.pattern().length()));

        QRegExp rxStartName(">");
        QRegExp rxEndName("</a");

        int nameStart = rxStartName.indexIn(str, linkEnd);
        int nameEnd= rxEndName.indexIn(str, nameStart);
        if(nameStart == -1 || nameEnd == -1)
        {
            QMessageBox::warning(0, "warning!", "failed to fetch name at: ", str.mid(nameStart, str.size() - nameStart));
        }
        QString name = str.mid(nameStart + rxStartName.pattern().length(),
                               nameEnd - (nameStart + rxStartName.pattern().length()));

        //qDebug()  << name << " " << link << " " << counter++;
        indexStart = linkEnd;
        names[currentProcessedSetion].append(Fandom{name, link, currentProcessedSetion});
    }
    managerEventLoop.quit();
}

void MainWindow::OnCrossoverReply(QNetworkReply * reply)
{
    QByteArray data=reply->readAll();
    qDebug() << reply->error();
    QString str(data);
    reply->deleteLater();
    // getting to the start of fandom section
    int indexOfSlash = currentProcessedSetion.indexOf("/");
    QString pattern = currentProcessedSetion.mid(indexOfSlash + 1, currentProcessedSetion.length() - (indexOfSlash +1)) + "\\sCrossovers";
    QRegExp rxStartFandoms(pattern);
    int indexStart = rxStartFandoms.indexIn(str);
    if(indexStart == -1)
    {
        QMessageBox::warning(0, "warning!", "failed to find the start of fandom section");
        return;
    }

    QRegExp rxEndFandoms("</TABLE>");
    int indexEnd= rxEndFandoms.indexIn(str);
    if(indexEnd == -1)
    {
        QMessageBox::warning(0, "warning!", "failed to find the end of fandom section");
        return;
    }
    while(true)
    {
        QRegExp rxStartLink("href=\"");
        QRegExp rxEndLink("/\"");

        int linkStart = rxStartLink.indexIn(str, indexStart);
        if(linkStart == -1)
            break;
        int linkEnd= rxEndLink.indexIn(str, linkStart);
        if(linkStart == -1 || linkEnd == -1)
        {
            QMessageBox::warning(0, "warning!", "failed to fetch link at: ", str.mid(linkStart, str.size() - linkStart));
        }
        QString link = str.mid(linkStart + rxStartLink.pattern().length(),
                               linkEnd - (linkStart + rxStartLink.pattern().length()));

        QRegExp rxStartName(">");
        QRegExp rxEndName("</a");

        int nameStart = rxStartName.indexIn(str, linkEnd);
        int nameEnd= rxEndName.indexIn(str, nameStart);
        if(nameStart == -1 || nameEnd == -1)
        {
            QMessageBox::warning(0, "warning!", "failed to fetch name at: ", str.mid(nameStart, str.size() - nameStart));
        }
        QString name = str.mid(nameStart + rxStartName.pattern().length(),
                               nameEnd - (nameStart + rxStartName.pattern().length()));

        //qDebug()  << name << " " << link << " " << counter++;
        indexStart = linkEnd;
        names[currentProcessedSetion].append(Fandom{name, link, currentProcessedSetion});
    }
    managerEventLoop.quit();
}

void MainWindow::on_pbCrawl_clicked()
{

    pageCounter = 0;
    ui->edtResults->clear();
    processedCount = 0;
    nextUrl = QString();
    if((ui->cbCrossovers->currentText().isEmpty()
            && ui->cbNormals->currentText().isEmpty())
            ||(!ui->cbCrossovers->currentText().isEmpty()
               && !ui->cbNormals->currentText().isEmpty())
            )
    {
        QMessageBox::warning(0, "warning!", "Please, select normal OR crossover category");
        return;
    }
    QString url;
    if(!ui->cbCrossovers->currentText().isEmpty())
    {
        url = GetCrossoverUrl(ui->cbCrossovers->currentText());
        lastUpdated = GetMaxUpdateDateForSection(ui->cbCrossovers->currentText() + "CROSSOVERS");
    }
    else
    {
        url = GetNormalUrl(ui->cbCrossovers->currentText());
        lastUpdated = GetMaxUpdateDateForSection(ui->cbNormals->currentText());
    }
    //SkipPages(ui->leStartFromPage->text().toInt()-1);
    RequestPage(url);
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

void MainWindow::OnSectionChanged(QString)
{
//    UpdateFandomList();
//    UpdateCrossoverList();
    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));
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

void MainWindow::on_pbInit_clicked()
{
    Init();
}
