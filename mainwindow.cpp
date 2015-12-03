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
    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(OnSetTag(QString)));
    QAction* hideAction = browserMenu.addAction("Hide fanfic", mapper, SLOT(map()));
    QAction* smutAction = browserMenu.addAction("Smut fanfic", mapper, SLOT(map()));
    QAction* unknownFandom = browserMenu.addAction("Unknown Fandom", mapper, SLOT(map()));
    QAction* readQueue = browserMenu.addAction("Add to reading queue", mapper, SLOT(map()));
    QAction* meh = browserMenu.addAction("Meh description", mapper, SLOT(map()));

    QAction* crapFandom = browserMenu.addAction("Crap Fandom", mapper, SLOT(map()));
    QAction* reading = browserMenu.addAction("Already reading", mapper, SLOT(map()));
    QAction* finished = browserMenu.addAction("Finished", mapper, SLOT(map()));
    QAction* disgusting= browserMenu.addAction("Disgusting", mapper, SLOT(map()));

    mapper->setMapping(hideAction,   QString("hidden"));
    mapper->setMapping(smutAction,   QString("smut"));
    mapper->setMapping(unknownFandom, QString("unknown"));
    mapper->setMapping(readQueue, QString("queue"));
    mapper->setMapping(meh, QString("meh_description"));
    mapper->setMapping(crapFandom, QString("crap_fandom"));
    mapper->setMapping(reading, QString("reading"));
    mapper->setMapping(finished, QString("finished"));
    mapper->setMapping(disgusting, QString("disgusting"));

    ui->edtResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->edtResults, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnShowContextMenu(QPoint)));
    connect(&fandomManager, &QNetworkAccessManager::finished, this, &MainWindow::OnFandomReply);
    connect(&crossoverManager, &QNetworkAccessManager::finished, this, &MainWindow::OnCrossoverReply);
    connect(ui->cbSectionTypes, SIGNAL(currentTextChanged(QString)), this, SLOT(OnSectionChanged(QString)));
    nameOfFandomSectionToLink["Normal/Anime/Manga"] = QString::fromLocal8Bit("https://www.fanfiction.net/anime/");
    nameOfFandomSectionToLink["Normal/Book"] = QString::fromLocal8Bit("https://www.fanfiction.net/book/");
    nameOfFandomSectionToLink["Normal/Cartoon"] = QString::fromLocal8Bit("https://www.fanfiction.net/cartoon/");
    nameOfCrossoverSectionToLink["Crossover/Anime/Manga"] = QString::fromLocal8Bit("https://www.fanfiction.net/crossovers/anime/");
    nameOfCrossoverSectionToLink["Crossover/Book"] = QString::fromLocal8Bit("https://www.fanfiction.net/crossovers/book/");
    nameOfCrossoverSectionToLink["Crossover/Cartoon"] = QString::fromLocal8Bit("https://www.fanfiction.net/crossovers/cartoon/");


    connect(ui->buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnCheckboxFilter(int)));

    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));

    pbMain = new QProgressBar;
    pbMain->setMaximumWidth(200);
    lblCurrentOperation = new QLabel;
    lblCurrentOperation->setMaximumWidth(300);

    ui->statusBar->addPermanentWidget(lblCurrentOperation,1);
    ui->statusBar->addPermanentWidget(pbMain,0);

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
        currentPosition = section.start;

        section.fandom = ui->cbCrossovers->currentText().isEmpty() ? ui->cbNormals->currentText() : ui->cbCrossovers->currentText() + " CROSSOVER";
        GetUrl(section, currentPosition, str);
        GetTitle(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);

        GetStatSection(section, currentPosition, str);

        GetTaggedSection(section.statSection.replace(",", ""), "Words:\\s(\\d{6})", [&section](QString val){ section.wordCount = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Chapters:\\s(\\d{1,5})", [&section](QString val){ section.chapters = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Reviews:\\s(\\d{1,5})", [&section](QString val){ section.reviews = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Favs:\\s(\\d{1,5})", [&section](QString val){ section.favourites = val;});
        GetTaggedSection(section.statSection, "Published:\\s<span\\sdata-xutime='(\\d+)'", [&section](QString val){
            if(val != "not found")
                section.published.setTime_t(val.toInt()); ;
        });
        GetTaggedSection(section.statSection, "Updated:\\s<span\\sdata-xutime='(\\d+)'", [&section](QString val){
            if(val != "not found")
                section.updated.setTime_t(val.toInt());
            else
                section.updated.setTime_t(0);
        });
        GetTaggedSection(section.statSection, "Rated:\\s(.{1})", [&section](QString val){ section.rated = val;});
        GetTaggedSection(section.statSection, "English\\s-\\s([A-Za-z/\\-]+)\\s-\\sChapters", [&section](QString val){ section.genre = val;});
        GetTaggedSection(section.statSection, "</span>\\s-\\s([A-Za-z\\.\\s/]+)$", [&section](QString val){ section.characters = val;});

        if(section.fandom.contains("CROSSOVER"))
            GetCrossoverFandomList(section, currentPosition, str);

        if(section.updated.toMSecsSinceEpoch() < lastUpdated.toMSecsSinceEpoch())
            abort = true;
        if(section.isValid)
        {
            sections.append(section);
        }

    }
    qDebug() << "page: " << processedCount;

    if(sections.size() == 0)
    {
        ui->edtResults->insertHtml("<span> No data found on the page.<br></span>");
        ui->edtResults->insertHtml("<span> \nFinished loading data <br></span>");
        return;
    }

    auto url = GetCurrentFilterUrl();
    for(auto section : sections)
    {
        section.origin = url;
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
    //ui->edtResults->insertHtml("<span> \n Next page \n <br></span>");

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

void MainWindow::GetStatSection(Section &section, int &startfrom, QString text)
{
    QRegExp rxStart("padtop2\\sxgray");
    QRegExp rxEnd("</div></div></div>");
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.statSection= text.mid(indexStart + 15,indexEnd - (indexStart + 15));
    section.statSectionStart = indexStart + 15;
    section.statSectionEnd = indexEnd;
    //qDebug() << section.statSection;
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

void MainWindow::GetTaggedSection(QString text, QString rxString ,std::function<void (QString)> functor)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    //qDebug() << rx.capturedTexts();
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
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

QString MainWindow::CreateURL(QString str)
{
    return "https://www.fanfiction.net/" + str;
}

void MainWindow::LoadData()
{
    if(ui->cbMinWordCount->currentText().trimmed().isEmpty())
    {
        QMessageBox::warning(0, "warning!", "Please set minimum word count");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString queryString = "select rowid, f.* from fanfics f where 1 = 1 " ;
    if(ui->cbMinWordCount->currentText().toInt() > 0)
        queryString += " and wordcount > :minwordcount ";
    if(ui->cbMaxWordCount->currentText().toInt() > 0)
        queryString += " and wordcount < :maxwordcount ";
    if(!ui->leContainsGenre->text().isEmpty())
        for(auto genre : ui->leContainsGenre->text().split(" "))
            queryString += QString(" AND genres like '%%1%' ").arg(genre);

    if(!ui->leNotContainsGenre->text().isEmpty())
        for(auto genre : ui->leNotContainsGenre->text().split(" "))
            queryString += QString(" AND genres not like '%%1%' ").arg(genre);

    QString tags, not_tags;

    if(ui->chkSmut->isChecked())
        tags+=WrapTag("smut") + "|";
    else if(ui->chkUnknownFandoms->isChecked())
        tags+=WrapTag("unknown") + "|";
    else if(ui->chkMeh->isChecked())
        tags+=WrapTag("meh_description") + "|";
    else if(ui->chkReadQueue->isChecked())
        tags+=WrapTag("queue") + "|";
    else if(ui->chkReading->isChecked())
        tags+=WrapTag("reading") + "|";
    else if(ui->chkFinished->isChecked())
        tags+=WrapTag("finished") + "|";
    else if(ui->chkDisgusting->isChecked())
        tags+=WrapTag("disgusting") + "|";
    else if(ui->chkCrapFandom->isChecked())
        tags+=WrapTag("crap_fandom") + "|";


    if(!ui->cbNormals->currentText().isEmpty())
        queryString+=QString(" and  fandom like '%%1%'").arg(ui->cbNormals->currentText());
    else if(!ui->cbCrossovers->currentText().isEmpty())
        queryString+=QString(" and  fandom like '%%1%'").arg(ui->cbCrossovers->currentText());


    tags.chop(1);
    not_tags.chop(1);

    if(!tags.isEmpty())
        queryString+=" and tags regexp :tags";
    else
        queryString+=" and tags = ' none ' ";

    queryString+=" ORDER BY WORDCOUNT DESC ";
    QSqlQuery q(db);
    q.prepare(queryString);

    if(ui->cbMinWordCount->currentText().toInt() > 0)
        q.bindValue(":minwordcount", ui->cbMinWordCount->currentText().toInt());
    if(ui->cbMaxWordCount->currentText().toInt() > 0)
        q.bindValue(":maxwordcount", ui->cbMaxWordCount->currentText().toInt());

    q.bindValue(":tags", tags);
    q.bindValue(":not_tags", not_tags);

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
    ui->edtResults->setUpdatesEnabled(false);
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
    ui->edtResults->setUpdatesEnabled(true);
    qDebug() << "loaded fics:" << counter;

}


void MainWindow::OnSetTag(QString tag)
{
    bool ok = false;
    int value = ui->edtResults->textCursor().selectedText().trimmed().toInt(&ok);
    if(ok)
    {
        QString path = "CrawlerDB.sqlite";
        QSqlDatabase db = QSqlDatabase::database(path);//not dbConnection
        QSqlQuery q(db);
        q.prepare(QString("update fanfics set tags = tags || :tag where rowid = :id and tags not regexp :wrappedTag"));
        q.bindValue(":id", value);
        q.bindValue(":tag", " " + tag + " ");
        q.bindValue(":wrappedTag", WrapTag(tag));
        q.exec();
        if(q.lastError().isValid())
            qDebug() << q.lastError();
    }
    HideCurrentID();
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

        pbMain->setMinimum(0);
        pbMain->setMaximum(names[value].size());
        lblCurrentOperation->setText("Currently loading: " + value);
        int counter = 0;
        for(auto fandom : names[value])
        {
            counter++;
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
            if(counter%100 == 0)
            {
                pbMain->setValue(counter);
                QApplication::processEvents();
            }
        }
        pbMain->setValue(pbMain->maximum());
        QApplication::processEvents();
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

        pbMain->setMinimum(0);
        pbMain->setMaximum(names[value].size());
        lblCurrentOperation->setText("Currently loading: " + value);
        int counter = 0;
        for(auto fandom : names[value])
        {
            counter++;
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
            if(counter%100 == 0)
            {
                pbMain->setValue(counter);
                QApplication::processEvents();
            }
        }
        pbMain->setValue(pbMain->maximum());
        QApplication::processEvents();


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

        //qDebug() << "Inserting: " << section.author << " " << section.title << " " << section.fandom << " " << section.genre;
        QString query = "INSERT INTO FANFICS (FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, CHARACTERS, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, URL, ORIGIN) "
                        "VALUES (  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, :CHARACTERS, :RATED, :summary, :genres, :published, :updated, :url, :origin)";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author);
        q.bindValue(":title",section.title);
        q.bindValue(":wordcount",section.wordCount.toInt());
        q.bindValue(":CHAPTERS",section.chapters.trimmed().toInt());
        q.bindValue(":FAVOURITES",section.favourites.toInt());
        q.bindValue(":REVIEWS",section.reviews.toInt());
        q.bindValue(":CHARACTERS",section.characters);
        q.bindValue(":RATED",section.rated);

        q.bindValue(":summary",section.summary);
        q.bindValue(":genres",section.genre);
        q.bindValue(":published",section.published);
        q.bindValue(":updated",section.updated);
        q.bindValue(":url",section.url);
        q.bindValue(":origin",section.origin);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << "failed to insert: " << section.author << " " << section.title;
            qDebug() << q.lastError();
        }

    }

    if(isUpdate)
    {
        //qDebug() << "Updating: " << section.author << " " << section.title;
        QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, summary = :summary, genres= :genres, published = :published, updated = :updated, url = :url "
                        " where author = :author and title = :title";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author);
        q.bindValue(":title",section.title);

        q.bindValue(":wordcount",section.wordCount.toInt());
        q.bindValue(":CHAPTERS",section.chapters.trimmed().toInt());
        q.bindValue(":FAVOURITES",section.favourites.toInt());
        q.bindValue(":REVIEWS",section.reviews.toInt());
        q.bindValue(":CHARACTERS",section.characters);
        q.bindValue(":RATED",section.rated);

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

QString MainWindow::WrapTag(QString tag)
{
    tag= "(.{0,}" + tag + ".{0,})";
    return tag;
}

void MainWindow::HideCurrentID()
{
    auto cursorPosition = ui->edtResults->textCursor().position();
    auto cursor = ui->edtResults->textCursor();

    QString text = ui->edtResults->toPlainText();
    int posPrevious = text.midRef(0, cursorPosition - 70).lastIndexOf("ID:");

    int posNewline = text.indexOf("\n", posPrevious);
    if(posNewline == -1)
        posNewline = text.length();
    if(posPrevious == -1)
    {
        posPrevious = 0;
        posNewline = 0;
    }
    int posCurrent = text.indexOf("\n", cursorPosition);
    if(posCurrent == -1)
        posCurrent = text.length();

    cursor.setPosition(posNewline, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,posCurrent - posNewline + 1);
    cursor.deleteChar();
    ui->edtResults->setTextCursor(cursor);
}

QString MainWindow::GetCurrentFilterUrl()
{
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
    return url;
}

bool MainWindow::CheckSectionAvailability()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select count(fandom) from fandoms");
    QSqlQuery q(qs, db);
    q.next();
    if(q.value(0).toInt() == 0)
    {
        lblCurrentOperation->setText("Please, wait");
        QMessageBox::information(nullptr, "Attention!", "Section information is not available, the app will now load it from the internet.\nThis is a one time operation, unless you want to update it with \"Reload section data\"\nPlease wait until it finishes before doing anything.");
        Init();
        QMessageBox::information(nullptr, "Attention!", "Section data is initialized, the app is usable. Have fun searching.");
        pbMain->hide();
        lblCurrentOperation->hide();
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


void MainWindow::OnShowContextMenu(QPoint p)
{
    browserMenu.popup(this->mapToGlobal(p));
}

void MainWindow::OnSectionChanged(QString)
{
    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));
}

void MainWindow::on_pbLoadDatabase_clicked()
{
    LoadData();
}

void MainWindow::on_pbInit_clicked()
{
    Init();
}


void MainWindow::OnCheckboxFilter(int)
{
    LoadData();
}
