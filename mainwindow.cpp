#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QRegExp>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QPair>
#include <QPoint>
#include <QStringListModel>
#include <QDesktopServices>
#include <QTextCodec>
#include <QSettings>
#include "genericeventfilter.h"
#include <algorithm>

bool TagEditorHider(QObject* /*obj*/, QEvent *event, QWidget* widget)
{
    if(event->type() == QEvent::FocusOut)
    {
        QWidget* focused = QApplication::focusWidget();
        if(focused->parent() != widget)
        {
            widget->hide();
            return true;
        }
    }
    return false;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("ffnet sane search engine");
    ReadTags();
    connect(
                &manager, SIGNAL (finished(QNetworkReply*)),
                this, SLOT (OnNetworkReply(QNetworkReply*)));


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

    ui->edtResults->setOpenLinks(false);
    connect(ui->edtResults, &QTextBrowser::anchorClicked, this, OnLinkClicked);

//    tagEditor->setOpenLinks(false);
//    tagEditor->setFont(QFont("Verdana", 16));
//    connect(tagEditor, &QTextBrowser::anchorClicked, this, OnTagClicked);

    // should refer to new tag widget instead
    GenericEventFilter* eventFilter = new GenericEventFilter(this);
    eventFilter->SetEventProcessor(std::bind(TagEditorHider,std::placeholders::_1, std::placeholders::_2, tagWidgetDynamic));
    tagWidgetDynamic->installEventFilter(eventFilter);
    connect(tagWidgetDynamic, &TagWidget::tagToggled, this, OnTagToggled);

    connect(ui->wdgTagsPlaceholder, &TagWidget::refilter, [&](){
        if(ui->gbTagFilters->isChecked() && ui->wdgTagsPlaceholder->GetSelectedTags().size() > 0)
            LoadData();
    });

    connect(tagWidgetDynamic, &TagWidget::tagDeleted, [&](QString tag){
        ui->wdgTagsPlaceholder->OnRemoveTagFromEdit(tag);
    });
    connect(tagWidgetDynamic, &TagWidget::tagAdded, [&](QString tag){
        ui->wdgTagsPlaceholder->OnNewTag(tag, false);

    });

    ui->wdgTagsPlaceholder->SetAddDialogVisibility(false);
    this->setAttribute(Qt::WA_QuitOnClose);
    ReadSettings();
}


MainWindow::~MainWindow()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.setValue("Settings/minWordCount", ui->cbMinWordCount->currentText());
    settings.setValue("Settings/maxWordCount", ui->cbMaxWordCount->currentText());
    settings.setValue("Settings/normals", ui->cbNormals->currentText());
    settings.setValue("Settings/crossovers", ui->cbCrossovers->currentText());
    settings.setValue("Settings/plusGenre", ui->leContainsGenre->text());
    settings.setValue("Settings/minusGenre", ui->leNotContainsGenre->text());
    settings.setValue("Settings/plusWords", ui->leContainsWords->text());
    settings.setValue("Settings/minusWords", ui->leNotContainsWords->text());
    settings.setValue("Settings/section", ui->cbSectionTypes->currentText());
    settings.setValue("Settings/active", ui->chkActive->isChecked());
    settings.setValue("Settings/completed", ui->chkComplete->isChecked());
    settings.setValue("Settings/filterOnTags", ui->gbTagFilters->isChecked());
    //settings.setValue("Settings/spMain", ui->spMain->saveState());
    QString temp;
    for(int size : ui->spMain->sizes())
        temp.push_back(QString::number(size) + " ");
    settings.setValue("Settings/spMain", temp.trimmed());
    settings.sync();
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

        GetTaggedSection(section.statSection.replace(",", ""), "Words:\\s(\\d{1,8})", [&section](QString val){ section.wordCount = val;});
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
        GetTaggedSection(section.statSection, "</span>\\s-\\s([A-Za-z\\.\\s/]+)$", [&section](QString val){
            section.characters = val.replace(" - Complete", "");
        });
        GetTaggedSection(section.statSection, "(Complete)$", [&section](QString val){
            if(val != "not found")
                section.complete = 1;
        });

        if(section.fandom.contains("CROSSOVER"))
            GetCrossoverFandomList(section, currentPosition, str);

        if(section.updated.toMSecsSinceEpoch() < lastUpdated.toMSecsSinceEpoch() && !ignoreUpdateDate)
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


    for(auto section : sections)
    {
        section.origin = currentFilterurl;
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
    qDebug() << section.statSection;
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

    for(QString word: ui->leContainsWords->text().split(" "))
    {
        if(word.trimmed().isEmpty())
            continue;
        queryString += QString(" AND summary like '%%1%' and summary not like '%not %1%' ").arg(word);
    }

    for(QString word: ui->leNotContainsWords->text().split(" "))
    {
        if(word.trimmed().isEmpty())
            continue;
        queryString += QString(" AND summary not like '%%1%' and summary not like '%not %1%").arg(word);
    }

    QString tags, not_tags;


    QString diffField = " WORDCOUNT DESC";
    bool tagsMatter = true;
    if(ui->chkLongestRunning->isChecked())
    {
        queryString =  "select rowid, julianday(f.updated) - julianday(f.published) as datediff, f.* from fanfics f where 1 = 1 and datediff > 0  ";
        diffField = "datediff desc";
        tagsMatter = false;
    }
    if(ui->chkBehemothChapters->isChecked())
    {
        queryString = "select rowid, f.wordcount / f.chapters as datediff, f.* from fanfics f where 1 = 1";
        diffField = "datediff desc";
        tagsMatter = false;
    }
    if(ui->chkMinigun->isChecked())
    {
        queryString = "select rowid, f.wordcount / f.chapters as datediff, f.* from fanfics f where 1 = 1";
        diffField = "datediff asc";
        tagsMatter = false;
    }
    if(ui->chkPokemon->isChecked())
    {
        queryString = "select rowid, f.* from fanfics f where 1 = 1 and length(characters) > 40 ";
        diffField = "length(characters) desc";
        tagsMatter = false;
    }
    if(ui->chkTLDR->isChecked())
    {
        queryString = "select rowid, f.* from fanfics f where 1 = 1 and summary not like '%sequel%'"
                      "and summary not like '%revision%'"
                      "and summary not like '%rewrite%'"
                      "and summary not like '%abandoned%'"
                      "and summary not like '%part%'"
                      "and summary not like '%complete%'"
                      "and summary not like '%adopted%'"
                      "and length(summary) < 100 "
                      "and summary not like '%discontinued%'";
        diffField = "length(summary) asc";
        tagsMatter = false;
    }
    if(ui->chkComplete->isChecked())
        queryString+=QString(" and  complete = 1");

    if(ui->chkActive->isChecked())
        queryString+=QString(" and cast("
                             "("
                             " strftime('%s',f.updated)-strftime('%s',CURRENT_TIMESTAMP) "
                             " ) AS real "
                             " )/60/60/24 >-365");

    if(!ui->cbNormals->currentText().isEmpty())
        queryString+=QString(" and  fandom like '%%1%'").arg(ui->cbNormals->currentText());
    else if(!ui->cbCrossovers->currentText().isEmpty())
        queryString+=QString(" and  fandom like '%%1%'").arg(ui->cbCrossovers->currentText());


    for(auto tag : ui->wdgTagsPlaceholder->GetSelectedTags())
        tags.push_back(WrapTag(tag) + "|");

    tags.chop(1);
    not_tags.chop(1);

    if(tagsMatter)
    {
        if(!tags.isEmpty())
            queryString+=" and tags regexp :tags ";
        else
            queryString+=" and tags = ' none ' ";
    }

    queryString+="COLLATE NOCASE ORDER BY " + diffField;
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
        if(ui->chkUrls->isChecked())
        {
            QString toInsert = "<a href=\"" + CreateURL((q.value("URL").toString()) + "\"> %1 </a><br>");
            toInsert= toInsert.arg(CreateURL((q.value("URL").toString())));
            ui->edtResults->insertHtml(toInsert);
            continue;
        }
        QString toInsert = "<a href=\"" + CreateURL((q.value("URL").toString()) + "\"> %1 </a>");
        toInsert= toInsert.arg(q.value("TITLE").toString());
        ui->edtResults->insertHtml(toInsert);
        ui->edtResults->append("<span>" + QString("By: ") + q.value("AUTHOR").toString() + "</span>");
        ui->edtResults->append("<span>" + QString(" ") + q.value("SUMMARY").toString() + "</span>");
        ui->edtResults->append("<span>" + QString("<font color=\"green\">Genre: ") + q.value("GENRES").toString() + "</font></span>");
        ui->edtResults->append("<span>" + QString("<font color=\"green\">Words: ") + q.value("WORDCOUNT").toString() + "</font></span>");
        //ui->edtResults->append("<span>" + QString("ID: ") +  QString::number(q.value("rowid").toInt()) +  "</font></span>");


        QString toInsertTagOpener = QString("<br><a href=\"") + QString::number(q.value("rowid").toInt()) + "TAGS" + q.value("TAGS").toString() + QString(" \"> %1</a>");
        toInsertTagOpener= toInsertTagOpener.arg("Tags:");

        ui->edtResults->insertHtml(toInsertTagOpener);
        ui->edtResults->insertHtml(" <span>" + q.value("TAGS").toString().replace(" none ", "").trimmed().replace(" ", ",")+ "</span>");


        ui->edtResults->insertPlainText("\n");
        ui->edtResults->insertPlainText("\n");
        if(counter%100 == 0)
            qDebug() << "tick " << counter/100;
    }
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
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

void MainWindow::OpenTagWidget(QPoint pos, QString url)
{
    url = url.replace(" none ", "");
    QStringList temp = url.split("TAGS");
    QString id = temp.at(0);
    QString tags= temp.at(1);

    QList<QPair<QString, QString>> tagPairs;

    for(QString tag: ui->wdgTagsPlaceholder->GetAllTags())
    {
        if(tags.contains(tag))
            tagPairs.push_back({"1", tag});
        else
            tagPairs.push_back({"0", tag});
    }

    tagWidgetDynamic->InitFromTags(id.toInt(), tagPairs);
    tagWidgetDynamic->resize(500,200);
    //tagWidgetDynamic->move(ui->edtResults->mapTo(ui->twMain, QPoint(ui->edtResults->x(),0)).x(), pos.y());
    QPoint tempPoint(ui->edtResults->x(), 0);
    tempPoint = mapToGlobal(ui->twMain->mapTo(this, tempPoint));
    tagWidgetDynamic->move(tempPoint.x(), pos.y());
    tagWidgetDynamic->setWindowFlags(Qt::FramelessWindowHint);


    tagWidgetDynamic->show();
    tagWidgetDynamic->setFocus();
}

void MainWindow::ReadTags()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select tag from tags ");
    QSqlQuery q(qs, db);
    QList<QPair<QString, QString>> tagPairs;
    while(q.next())
    {
        tagList.append(q.value(0).toString());
    }

    if(tagList.isEmpty())
    {
        QStringList temp;
        temp << "smut" << "hidden" << "meh_description" << "unknown_fandom" << "read_queue" << "reading" << "finished" << "disgusting" << "crap_fandom";
        for(QString tag : temp)
        {
            QString qs = QString("INSERT INTO TAGS (TAG) VALUES (:tag)");
            QSqlQuery q(db);
            q.prepare(qs);
            q.bindValue(":tag", tag);
            q.exec();
            if(q.lastError().isValid())
                qDebug() << q.lastError();
        }
        tagList = temp;
    }
    for(auto tag : tagList)
        tagPairs.push_back({ "0", tag });
    ui->wdgTagsPlaceholder->InitFromTags(-1, tagPairs);
}

void MainWindow::SetTag(int id, QString tag)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("update fanfics set tags = tags || ' ' || :tag where rowid = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag", tag);
    q.bindValue(":id",id);
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
}

void MainWindow::UnsetTag(int id, QString tag)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);

    QString qs = QString("select tags from fanfics where rowid = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    q.exec();
    q.next();

    if(q.lastError().isValid())
        qDebug() << q.lastError();

    QStringList originaltags = q.value(0).toString().split(" ");
    originaltags.removeAll("none");
    originaltags.removeAll("");
    originaltags.removeAll(tag);


    qs = QString("update fanfics set tags = :tags where rowid = :id");

    QSqlQuery q1(db);
    q1.prepare(qs);
    q1.bindValue(":tags", " none " + originaltags.join(" "));
    q1.bindValue(":id", id);
    q1.exec();
    if(q1.lastError().isValid())
        qDebug() << q1.lastError();
}

QDateTime MainWindow::GetMaxUpdateDateForSection(QStringList sections)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select max(updated) as updated from fanfics where 1 = 1 %1");
    QString append;
    for(auto section : sections)
    {
        append+= QString(" and fandom like '%1' ").arg(section);
    }
    qs=qs.arg(append);
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
        QString query = "INSERT INTO FANFICS (FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, URL, ORIGIN) "
                        "VALUES (  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :url, :origin)";

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
        q.bindValue(":COMPLETE",section.complete);
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
        QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, summary = :summary, genres= :genres, published = :published, updated = :updated, url = :url "
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
        q.bindValue(":COMPLETE",section.complete);
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
        lastUpdated = GetMaxUpdateDateForSection(QStringList() << ui->cbCrossovers->currentText() << "CROSSOVERS");
    }
    else
    {
        url = GetNormalUrl(ui->cbCrossovers->currentText());
        lastUpdated = GetMaxUpdateDateForSection(QStringList() << ui->cbNormals->currentText());
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
    return true;
}

void MainWindow::ReadSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    ui->cbCrossovers->setCurrentText(settings.value("Settings/crossovers", "").toString());
    ui->cbNormals->setCurrentText(settings.value("Settings/normals", "").toString());
    ui->cbMaxWordCount->setCurrentText(settings.value("Settings/maxWordCount", "").toString());
    ui->cbMinWordCount->setCurrentText(settings.value("Settings/minWordCount", 100000).toString());

    ui->leContainsGenre->setText(settings.value("Settings/plusGenre", "").toString());
    ui->leNotContainsGenre->setText(settings.value("Settings/minusGenre", "").toString());
    ui->leNotContainsWords->setText(settings.value("Settings/minusWords", "").toString());
    ui->leContainsWords->setText(settings.value("Settings/plusWords", "").toString());


    ui->chkActive->setChecked(settings.value("Settings/active", false).toBool());
    ui->chkComplete->setChecked(settings.value("Settings/completed", false).toBool());
    ui->gbTagFilters->setChecked(settings.value("Settings/filterOnTags", false).toBool());
    QString temp = settings.value("Settings/spMain", false).toString();
    QList<int>  sizes;
    for(auto tmp : temp.split(" "))
        if(!tmp.isEmpty())
            sizes.push_back(tmp.toInt());

    ui->spMain->setSizes(sizes);
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
    currentFilterurl = GetCurrentFilterUrl();
    pageCounter = 0;
    ui->edtResults->clear();
    processedCount = 0;
    ignoreUpdateDate = false;
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
        lastUpdated = GetMaxUpdateDateForSection(QStringList() << ui->cbCrossovers->currentText() << "CROSSOVERS");
    }
    else
    {
        url = GetNormalUrl(ui->cbCrossovers->currentText());
        lastUpdated = GetMaxUpdateDateForSection(QStringList() << ui->cbNormals->currentText() );
    }
    //SkipPages(ui->leStartFromPage->text().toInt()-1);
    if(ui->sbStartFrom->value() != 0)
    {
        url+="&p=" + QString::number(ui->sbStartFrom->value());
        ignoreUpdateDate = true;
    }
    RequestPage(url);
}

void MainWindow::OnLinkClicked(const QUrl & url)
{
    if(url.toString().contains("fanfiction.net"))
        QDesktopServices::openUrl(url);
    else
        OpenTagWidget(QCursor::pos(), url.toString());
}


void MainWindow::OnTagToggled(int id, QString tag, bool checked)
{
    if(checked)
        SetTag(id, tag);
    else
        UnsetTag(id, tag);
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
