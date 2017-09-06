#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GlobalHeaders/SingletonHolder.h"
#include "GlobalHeaders/simplesettings.h"
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
#include <QSortFilterProxyModel>
#include <QQuickWidget>
#include <QDebug>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>
#include <QSqlDriver>
#include <QClipboard>
#include <chrono>
#include <algorithm>

#include "genericeventfilter.h"

#include <algorithm>
#include "include/init_database.h"
#include "include/favparser.h"
#include "include/fandomparser.h"
#include "include/url_utils.h"

struct SplitPart
{
    QString data;
    int partId;
};

struct SplitJobs
{
    QVector<SplitPart> parts;
    int storyCountInWhole;
    QString authorName;
};


QString ParseAuthorNameFromFavouritePage(QString data)
{
    QString result;
    QRegExp rx("title>([A-Za-z0-9.\\-\\s]+)(?=\\s|\\sFanFiction)");
    //rx.setMinimal(true);
    int index = rx.indexIn(data);
    if(index == -1)
        return result;
    //qDebug() << rx.capturedTexts();
    result = rx.cap(1);
    return result;
}

SplitJobs SplitJob(QString data)
{
    SplitJobs result;
    int threadCount = QThread::idealThreadCount();
    QRegExp rxStart("<div\\sclass=\'z-list\\sfavstories\'");
    int index = rxStart.indexIn(data);
    int captured = data.count(rxStart);
    result.storyCountInWhole = captured;
    qDebug() << "Will process "  << captured << " stories";

    int partSize = captured/(threadCount-1);
    qDebug() << "In packs of "  << partSize;
    index = 0;

    if(partSize < 70)
        partSize = 70;

    QList<int> splitPositions;
    int counter = 0;
    do{
        index = rxStart.indexIn(data, index+1);
        if(counter%partSize == 0 && index != -1)
        {
            splitPositions.push_back(index);
        }
        counter++;
    }while(index != -1);
    qDebug() << "Splitting into: "  << splitPositions;
    result.parts.reserve(splitPositions.size());
    for(int i = 0; i < splitPositions.size(); i++)
    {
        if(i != splitPositions.size()-1)
            result.parts.push_back({data.mid(splitPositions[i], splitPositions[i+1] - splitPositions[i]), i});
        else
            result.parts.push_back({data.mid(splitPositions[i], data.length() - splitPositions[i]),i});
    }
    return result;
}

void InsertLogIntoEditor(QTextEdit* edit, QString url)
{
    QString toInsert = "<a href=\"" + url + "\"> %1 </a>";
    toInsert= toInsert.arg(url);
    edit->append("<span>Processing url: </span>");
    if(toInsert.trimmed().isEmpty())
        toInsert=toInsert;
    edit->insertHtml(toInsert);
    QCoreApplication::processEvents();
}
bool TagEditorHider(QObject* /*obj*/, QEvent *event, QWidget* widget)
{
    if(event->type() == QEvent::FocusOut)
    {
        QWidget* focused = QApplication::focusWidget();

        if(focused->parent()->parent() != widget)
        {
            widget->hide();
            return true;
        }
    }
    return false;
}
QString NameOfFandomSectionToLink(QString val)
{
    return "https://www.fanfiction.net/" + val + "/";
}
QString NameOfCrossoverSectionToLink(QString val)
{
    return "https://www.fanfiction.net/crossovers/" + val + "/";
}
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    queryBuilder.SetIdRNGgenerator(new core::DefaultRNGgenerator());
    ui->chkShowDirectRecs->setVisible(false);
    ui->pbFirstWave->setVisible(false);

    this->setWindowTitle("ffnet sane search engine");
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(settings.value("Settings/hideCache", true).toBool())
        ui->chkCacheMode->setVisible(false);

    ui->dteFavRateCut->setDate(QDate::currentDate().addDays(-366));
    ui->pbLoadDatabase->setStyleSheet("QPushButton {background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   stop:0 rgba(179, 229, 160, 128), stop:1 rgba(98, 211, 162, 128))}"
                                      "QPushButton:hover {background-color: #9cf27b; border: 1px solid black;border-radius: 5px;}"
                                      "QPushButton {background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   stop:0 rgba(179, 229, 160, 128), stop:1 rgba(98, 211, 162, 128))}");

    ProcessRecommendationListsFromDB(database::GetAvailableRecommendationLists());
    ReadTags();
    recentFandomsModel = new QStringListModel;
    recommendersModel= new QStringListModel;
    qRegisterMetaType<WebPage>("WebPage");
    qRegisterMetaType<ECacheMode>("ECacheMode");


    ui->edtResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->edtResults, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnShowContextMenu(QPoint)));
    connect(ui->cbSectionTypes, SIGNAL(currentTextChanged(QString)), this, SLOT(OnSectionChanged(QString)));
    connect(ui->pbWipeFandom, SIGNAL(clicked(bool)), this, SLOT(WipeSelectedFandom(bool)));
    connect(ui->pbCopyAllUrls, SIGNAL(clicked(bool)), this, SLOT(OnCopyAllUrls()));

    sections.insert("Anime/Manga", core::Fandom{"Anime/Manga", "anime", NameOfFandomSectionToLink("anime"), NameOfCrossoverSectionToLink("anime")});
    sections.insert("Misc", core::Fandom{"Misc", "misc", NameOfFandomSectionToLink("misc"), NameOfCrossoverSectionToLink("misc")});
    sections.insert("Books", core::Fandom{"Books", "book", NameOfFandomSectionToLink("book"), NameOfCrossoverSectionToLink("book")});
    sections.insert("Movies", core::Fandom{"Movies", "movie", NameOfFandomSectionToLink("movie"), NameOfCrossoverSectionToLink("movie")});
    sections.insert("Cartoons", core::Fandom{"Cartoons", "cartoon", NameOfFandomSectionToLink("cartoon"), NameOfCrossoverSectionToLink("cartoon")});
    sections.insert("Comics", core::Fandom{"Comics", "comic", NameOfFandomSectionToLink("comic"), NameOfCrossoverSectionToLink("comic")});
    sections.insert("Games", core::Fandom{"Games", "game", NameOfFandomSectionToLink("game"), NameOfCrossoverSectionToLink("game")});
    sections.insert("Plays/Musicals", core::Fandom{"Plays/Musicals", "play", NameOfFandomSectionToLink("play"), NameOfCrossoverSectionToLink("play")});
    sections.insert("TV Shows", core::Fandom{"TV Shows", "tv", NameOfFandomSectionToLink("tv"), NameOfCrossoverSectionToLink("tv")});


    ui->cbNormals->setModel(new QStringListModel(database::GetFandomListFromDB(ui->cbSectionTypes->currentText())));

    pbMain = new QProgressBar;
    pbMain->setMaximumWidth(200);
    lblCurrentOperation = new QLabel;
    lblCurrentOperation->setMaximumWidth(300);

    ui->statusBar->addPermanentWidget(lblCurrentOperation,1);
    ui->statusBar->addPermanentWidget(pbMain,0);

    ui->edtResults->setOpenLinks(false);
    connect(ui->edtResults, &QTextBrowser::anchorClicked, this, &MainWindow::OnLinkClicked);

    // should refer to new tag widget instead
    GenericEventFilter* eventFilter = new GenericEventFilter(this);
    eventFilter->SetEventProcessor(std::bind(TagEditorHider,std::placeholders::_1, std::placeholders::_2, tagWidgetDynamic));
    tagWidgetDynamic->installEventFilter(eventFilter);
    connect(tagWidgetDynamic, &TagWidget::tagToggled, this, &MainWindow::OnTagToggled);

    connect(ui->wdgTagsPlaceholder, &TagWidget::refilter, [&](){
        qwFics->rootContext()->setContextProperty("ficModel", nullptr);

        if(ui->gbTagFilters->isChecked() && ui->wdgTagsPlaceholder->GetSelectedTags().size() > 0)
        {
            filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_fics);
            LoadData();
        }
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(fanfics);
        typetableModel->OnReloadDataFromInterface();
        qwFics->rootContext()->setContextProperty("ficModel", typetableModel);
    });

    connect(ui->wdgTagsPlaceholder, &TagWidget::tagDeleted, [&](QString tag){
        ui->wdgTagsPlaceholder->OnRemoveTagFromEdit(tag);

        if(tagList.contains(tag))
        {
            QSqlDatabase db = QSqlDatabase::database();

            QSqlQuery q(db);
            q.prepare("DELETE FROM TAGS where tag = :tag");
            q.bindValue(":tag", tag);
            q.exec();
            if(q.lastError().isValid())
                qDebug() << q.lastError();
            qwFics->rootContext()->setContextProperty("tagModel", tagList);
        }
    });
    connect(ui->wdgTagsPlaceholder, &TagWidget::tagAdded, [&](QString tag){
        //ui->wdgTagsPlaceholder->OnNewTag(tag, false);
        if(!tagList.contains(tag))
        {
            QSqlDatabase db = QSqlDatabase::database();

            QSqlQuery q(db);
            q.prepare("INSERT INTO TAGS(TAG) VALUES(:tag)");
            q.bindValue(":tag", tag);
            q.exec();
            if(q.lastError().isValid())
                qDebug() << q.lastError();
            tagList.append(tag);
            qwFics->rootContext()->setContextProperty("tagModel", tagList);
            FillRecTagBuildCombobox();
        }

    });

    this->setAttribute(Qt::WA_QuitOnClose);
    ReadSettings();
    SetupFanficTable();
    InitConnections();
    database::RebaseFandoms();

    recentFandomsModel->setStringList(database::FetchRecentFandoms());
    ui->lvTrackedFandoms->setModel(recentFandomsModel);
    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));
    ui->lvRecommenders->setModel(recommendersModel);
    connect(ui->lvTrackedFandoms->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::OnNewSelectionInRecentList);
    connect(ui->lvRecommenders->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::OnNewSelectionInRecommenderList);
    CreatePageThreadWorker();

}


#define ADD_STRING_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toString(); \
    } \
    ); \

#define ADD_DATE_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toDateTime(); \
    } \
    ); \

#define ADD_INTEGER_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toInt(); \
    } \
    ); \

void MainWindow::SetupTableAccess()
{
    //    holder->SetColumns(QStringList() << "fandom" << "author" << "title" << "summary" << "genre" << "characters" << "rated"
    //                       << "published" << "updated" << "url" << "tags" << "wordCount" << "favourites" << "reviews" << "chapters" << "complete" << "atChapter" );
    ADD_STRING_GETSET(holder, 0, 0, fandom);
    ADD_STRING_GETSET(holder, 1, 0, author.name);
    ADD_STRING_GETSET(holder, 2, 0, title);
    ADD_STRING_GETSET(holder, 3, 0, summary);
    ADD_STRING_GETSET(holder, 4, 0, genre);
    ADD_STRING_GETSET(holder, 5, 0, characters);
    ADD_STRING_GETSET(holder, 6, 0, rated);
    ADD_DATE_GETSET(holder, 7, 0, published);
    ADD_DATE_GETSET(holder, 8, 0, updated);
    ADD_STRING_GETSET(holder, 9, 0, urlFFN);
    ADD_STRING_GETSET(holder, 10, 0, tags);
    ADD_INTEGER_GETSET(holder, 11, 0, wordCount);
    ADD_INTEGER_GETSET(holder, 12, 0, favourites);
    ADD_INTEGER_GETSET(holder, 13, 0, reviews);
    ADD_INTEGER_GETSET(holder, 14, 0, chapters);
    ADD_INTEGER_GETSET(holder, 15, 0, complete);
    ADD_INTEGER_GETSET(holder, 16, 0, atChapter);
    ADD_INTEGER_GETSET(holder, 17, 0, ID);
    ADD_INTEGER_GETSET(holder, 18, 0, recommendations);


    holder->AddFlagsFunctor(
                [](const QModelIndex& index)
    {
        if(index.column() == 8)
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        Qt::ItemFlags result;
        result |= Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return result;
    }
    );
}


void MainWindow::SetupFanficTable()
{
    holder = new TableDataListHolder<core::Fic>();
    typetableModel = new FicModel();

    SetupTableAccess();


    holder->SetColumns(QStringList() << "fandom" << "author" << "title" << "summary" << "genre" << "characters" << "rated" << "published"
                       << "updated" << "url" << "tags" << "wordCount" << "favourites" << "reviews" << "chapters" << "complete" << "atChapter" << "ID" << "recommendations");

    typetableInterface = QSharedPointer<TableDataInterface>(dynamic_cast<TableDataInterface*>(holder));

    typetableModel->SetInterface(typetableInterface);

    holder->SetData(fanfics);
    qwFics = new QQuickWidget();
    QHBoxLayout* lay = new QHBoxLayout;
    lay->addWidget(qwFics);
    ui->wdgFicviewPlaceholder->setLayout(lay);
    qwFics->setResizeMode(QQuickWidget::SizeRootObjectToView);
    qwFics->rootContext()->setContextProperty("ficModel", typetableModel);

    qwFics->rootContext()->setContextProperty("tagModel", tagList);
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    qwFics->rootContext()->setContextProperty("urlCopyIconVisible",
                                              settings.value("Settings/urlCopyIconVisible", true).toBool());
    QUrl source("qrc:/qml/ficview.qml");
    qwFics->setSource(source);

    QObject *childObject = qwFics->rootObject()->findChild<QObject*>("lvFics");
    connect(childObject, SIGNAL(chapterChanged(QVariant, QVariant, QVariant)), this, SLOT(OnChapterUpdated(QVariant, QVariant, QVariant)));
    connect(childObject, SIGNAL(tagClicked(QVariant, QVariant, QVariant)), this, SLOT(OnTagClicked(QVariant, QVariant, QVariant)));
    connect(childObject, SIGNAL(tagAdded(QVariant, QVariant)), this, SLOT(OnTagAdd(QVariant,QVariant)));
    connect(childObject, SIGNAL(tagDeleted(QVariant, QVariant)), this, SLOT(OnTagRemove(QVariant,QVariant)));
    connect(childObject, SIGNAL(urlCopyClicked(QString)), this, SLOT(OnCopyFicUrl(QString)));
    ui->deCutoffLimit->setDate(QDateTime::currentDateTime().date());
}
bool MainWindow::event(QEvent * e)
{
    switch(e->type())
    {
    case QEvent::WindowActivate :
        tagWidgetDynamic->hide();
        break ;
    default:
        break;
    } ;
    return QMainWindow::event(e) ;
}


MainWindow::~MainWindow()
{
    WriteSettings();
    delete ui;
}

void MainWindow::Init()
{
    names.clear();
    UpdateFandomList([](core::Fandom f){return f.url;});
    UpdateFandomList([](core::Fandom f){return f.crossoverUrl;});
    InsertFandomData(names);
    ui->cbNormals->setModel(new QStringListModel(database::GetFandomListFromDB(ui->cbSectionTypes->currentText())));
    ui->deCutoffLimit->setDate(QDateTime::currentDateTime().date());
}

void MainWindow::InitConnections()
{
    connect(ui->chkCustomFilter, &QCheckBox::clicked, this, &MainWindow::OnCustomFilterClicked);
    connect(ui->chkActivateReloadSectionData, &QCheckBox::clicked, this, &MainWindow::OnSectionReloadActivated);
    connect(ui->chkShowDirectRecs, &QCheckBox::clicked, this, &MainWindow::OnReloadRecLists);

}

void MainWindow::RequestAndProcessPage(QString fandom, QDateTime lastFandomUpdatedate, QString page, bool useLastIndex)
{
    nextUrl = page;
    //qDebug() << "will request url:" << nextUrl;
    if(ui->cbUseDateCutoff->isChecked())
        lastFandomUpdatedate = QDateTime(ui->deCutoffLimit->date());
    if(ui->chkIgnoreUpdateDate->isChecked())
        lastFandomUpdatedate = QDateTime();


    StartPageWorker();

    DisableAllLoadButtons();

    An<PageManager> pager;
    auto cacheMode = ui->chkCacheMode->isChecked() ? ECacheMode::use_cache : ECacheMode::dont_use_cache;
    qDebug() << "will request url:" << nextUrl;
    WebPage currentPage = pager->GetPage(nextUrl, cacheMode);
    FandomParser parser;
    QString lastUrl = parser.GetLast(currentPage.content);
    int pageCount = lastUrl.mid(lastUrl.lastIndexOf("=")+1).toInt();
    if(pageCount != 0)
    {
        pbMain->show();
        pbMain->setMaximum(pageCount);
    }
    qDebug() << "emitting page task:" << nextUrl << "\n" << lastUrl << "\n" << lastFandomUpdatedate;
    emit pageTask(nextUrl, lastUrl, lastFandomUpdatedate, ui->chkCacheMode->isChecked() ? ECacheMode::use_cache : ECacheMode::dont_use_cache);
    int counter = 0;
    WebPage webPage;
    do
    {
        while(pageQueue.isEmpty())
        {
            QThread::msleep(500);
            //qDebug() << "worker value is: " << worker->value;
            if(!worker->working)
                pageThread.start(QThread::HighPriority);
            QCoreApplication::processEvents();
        }

        webPage = pageQueue.at(0);
        pageQueue.pop_front();
        webPage.crossover = ui->rbCrossovers->isChecked();
        webPage.fandom =  fandom;
        webPage.type = EPageType::sorted_ficlist;
        auto startPageProcessing= std::chrono::high_resolution_clock::now();
        parser.ProcessPage(webPage);
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageProcessing;
        qDebug() << "Page processed in: " << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        if(webPage.source == EPageSource::network)
            pager->SavePageToDB(webPage);

        if(parser.minSectionUpdateDate < lastFandomUpdatedate && !ui->chkIgnoreUpdateDate->isChecked())
        {
            ui->edtResults->append("Already have updates past this point. Aborting.");
            break;
        }
        QCoreApplication::processEvents();

        if(pageCount == 0)
            pbMain->setValue((pbMain->value()+10)%pbMain->maximum());
        else
            pbMain->setValue(counter++);
        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();
        auto startPageRequest = std::chrono::high_resolution_clock::now();
        for(auto section: parser.processedStuff)
        {
            if(database::LoadIntoDB(section))
                processedFics++;
        }
        elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
        qDebug() << "Written into Db in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        db.commit();
    }while(!webPage.isLastPage);
    StopPageWorker();
    ShutdownProgressbar();
    EnableAllLoadButtons();
    //manager.get(QNetworkRequest(QUrl(page)));
}

WebPage MainWindow::RequestPage(QString pageUrl, ECacheMode cacheMode, bool autoSaveToDB)
{
    WebPage result;
    QString toInsert = "<a href=\"" + pageUrl + "\"> %1 </a>";
    toInsert= toInsert.arg(pageUrl);
    ui->edtResults->append("<span>Processing url: </span>");
    if(toInsert.trimmed().isEmpty())
        toInsert=toInsert;
    ui->edtResults->insertHtml(toInsert);

    //auto cacheMode = ui->chkCacheMode->isChecked() ? ECacheMode::use_cache : ECacheMode::dont_use_cache;

    pbMain->setTextVisible(false);
    pbMain->show();
    An<PageManager> pager;
    pager->SetDatabase(QSqlDatabase::database());
    result = pager->GetPage(pageUrl, cacheMode);
    if(autoSaveToDB)
        pager->SavePageToDB(result);
    return result;
}



inline core::Fic LoadFanfic(QSqlQuery& q)
{
    core::Fic result;
    result.ID = q.value("ID").toInt();
    result.fandom = q.value("FANDOM").toString();
    result.author.name = q.value("AUTHOR").toString();
    result.title = q.value("TITLE").toString();
    result.summary = q.value("SUMMARY").toString();
    result.genre= q.value("GENRES").toString();
    result.characters = q.value("CHARACTERS").toString().replace("not found", "");
    result.rated = q.value("RATED").toString();
    result.published = q.value("PUBLISHED").toDateTime();
    result.updated= q.value("UPDATED").toDateTime();
    result.SetUrl("ffn",q.value("URL").toString());
    result.tags = q.value("TAGS").toString();
    result.wordCount = q.value("WORDCOUNT").toString();
    result.favourites = q.value("FAVOURITES").toString();
    result.reviews = q.value("REVIEWS").toString();
    result.chapters = q.value("CHAPTERS").toString();
    result.complete= q.value("COMPLETE").toInt();
    result.wordCount = q.value("WORDCOUNT").toString();
    result.atChapter = q.value("AT_CHAPTER").toInt();
    result.recommendations= q.value("SUMRECS").toInt();
    //result.recommendations= 1;
    return std::move(result);
}

void MainWindow::LoadData()
{
    if(ui->cbMinWordCount->currentText().trimmed().isEmpty())
    {
        QMessageBox::warning(0, "warning!", "Please set minimum word count");
        return;
    }
    auto q = BuildQuery();
    q.setForwardOnly(true);
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
    fanfics.clear();
    while(q.next())
    {
        counter++;
        fanfics.push_back(LoadFanfic(q));

        if(counter%10000 == 0)
            qDebug() << "tick " << counter/1000;
        //qDebug() << "tick " << counter;
    }
    qDebug() << "loaded fics:" << counter;

}

QSqlQuery MainWindow::BuildQuery()
{
    QSqlDatabase db = QSqlDatabase::database();
    auto query = queryBuilder.Build(filter);
    QSqlQuery q(db);
    q.prepare(query.str);
    auto it = query.bindings.begin();
    auto end = query.bindings.end();
    while(it != end)
    {
        qDebug() << it.key() << " " << it.value();
        q.bindValue(it.key(), it.value());
        ++it;
    }
    return q;
}

QString MainWindow::BuildBias()
{
    QString result;
    if(ui->cbBiasFavor->currentText() == "None")
        return result;
    if(ui->cbBiasFavor->currentText() == "Favor")
        result += QString(" and ");
    else
        result += QString(" and not ");
    if(ui->cbBiasOperator->currentText() == ">")
        result += QString(" reviewstofavourites > ");
    else
        result += QString(" reviewstofavourites < ");
    result += ui->leBiasValue->text();
    return result;
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
        q.prepare(QString("update fanfics set tags = tags || :tag where ID = :id and not cfRegexp(:wrappedTag, tags)"));
        q.bindValue(":id", value);
        q.bindValue(":tag", " " + tag + " ");
        q.bindValue(":wrappedTag", WrapTag(tag));
        q.exec();
        if(q.lastError().isValid())
            qDebug() << q.lastError();
    }
    HideCurrentID();
}

void MainWindow::InsertFandomData(QMap<QPair<QString,QString>, core::Fandom> names)
{
    QSqlDatabase db = QSqlDatabase::database();
    QHash<QPair<QString, QString>, core::Fandom> knownValues;
    for(auto value : sections)
    {

        QString qs = QString("Select section, fandom, normal_url, crossover_url from fandoms where section = '%1'").
                arg(value.name.replace("'","''"));
        QSqlQuery q(qs, db);


        while(q.next())
        {
            knownValues[{q.value("section").toString(),
                    q.value("fandom").toString()}] =
                    core::Fandom{q.value("fandom").toString(),
                    q.value("section").toString(),
                    q.value("normal_url").toString(),
                    q.value("crossover_url").toString(),};
        }
        qDebug() << q.lastError();
    }
    auto make_key = [](core::Fandom f){return QPair<QString, QString>(f.section, f.name);} ;
    pbMain->setMinimum(0);
    pbMain->setMaximum(names.size());

    int counter = 0;
    QString prevSection;
    for(auto fandom : names)
    {
        counter++;
        if(prevSection != fandom.section)
        {
            lblCurrentOperation->setText("Currently loading: " + fandom.section);
            prevSection = fandom.section;
        }
        auto key = make_key(fandom);
        bool hasFandom = knownValues.contains(key);
        if(!hasFandom)
        {
            QString insert = "INSERT INTO FANDOMS (FANDOM, NORMAL_URL, CROSSOVER_URL, SECTION) "
                             "VALUES (:FANDOM, :URL, :CROSS, :SECTION)";
            QSqlQuery q(db);
            q.prepare(insert);
            q.bindValue(":FANDOM",fandom.name.replace("'","''"));
            q.bindValue(":URL",fandom.url.replace("'","''"));
            q.bindValue(":CROSS",fandom.crossoverUrl.replace("'","''"));
            q.bindValue(":SECTION",fandom.section.replace("'","''"));
            q.exec();
            if(q.lastError().isValid())
                qDebug() << q.lastError();
        }
        if(hasFandom && (
                    (knownValues[key].crossoverUrl.isEmpty() && !fandom.crossoverUrl.isEmpty())
                    || (knownValues[key].url.isEmpty() && !fandom.url.isEmpty())))
        {
            QString insert = "UPDATE FANDOMS set normal_url = :normal, crossover_url = :cross "
                             "where section = :section and fandom = :fandom";
            QSqlQuery q(db);
            q.prepare(insert);
            q.bindValue(":fandom",fandom.name.replace("'","''"));
            q.bindValue(":normal",fandom.url.replace("'","''"));
            q.bindValue(":cross",fandom.crossoverUrl.replace("'","''"));
            q.bindValue(":section",fandom.section.replace("'","''"));
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


void MainWindow::UpdateFandomList(std::function<QString(core::Fandom)> linkGetter)
{
    for(auto value : sections)
    {
        currentProcessedSection = value.name;

        An<PageManager> pager;
        WebPage result = pager->GetPage(linkGetter(value), ECacheMode::dont_use_cache);
        ProcessFandoms(result);

        //manager.get(QNetworkRequest(QUrl(linkGetter(value))));
        managerEventLoop.exec();
    }
}


QStringList MainWindow::GetCrossoverListFromDB()
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("Select fandom from fandoms where section = '%1' and crossover_url is not null").arg(ui->cbSectionTypes->currentText());
    QSqlQuery q(qs, db);
    QStringList result;
    result.append("");
    while(q.next())
    {
        result.append(q.value(0).toString());
    }
    return result;
}

QStringList MainWindow::GetCrossoverUrl(QString fandom, bool ignoreTrackingState)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("Select crossover_url from fandoms where fandom = '%1' ").arg(fandom);
    if(false)
        qs+=" and (tracked = 1 or tracked_crossovers = 1)";
    QSqlQuery q(qs, db);
    QStringList result;
    while(q.next())
    {
        QString rebindName = q.value(0).toString();
        QStringList temp = rebindName.split("/");

        rebindName = "/" + temp.at(2) + "-Crossovers" + "/" + temp.at(3);
        QString lastPart = "/0/?&srt=1&lan=1&r=10&len=%1";
        QSettings settings("settings.ini", QSettings::IniFormat);
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        int lengthCutoff = ui->cbWordCutoff->currentText() == "100k Words" ? 100 : 60;
        lastPart=lastPart.arg(lengthCutoff);
        QString resultString =  "https://www.fanfiction.net" + rebindName + lastPart;
        result.push_back(resultString);
    }
    qDebug() << result;
    return result;
}

QStringList MainWindow::GetNormalUrl(QString fandom, bool ignoreTrackingState)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("Select normal_url from fandoms where fandom = '%1' ").arg(fandom);
    if(false)
        qs+=" and (tracked = 1 or tracked_crossovers = 1)";
    QSqlQuery q(qs, db);
    QStringList result;
    while(q.next())
    {
        QString lastPart = "/?&srt=1&lan=1&r=10&len=%1";
        QSettings settings("settings.ini", QSettings::IniFormat);
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        int lengthCutoff = ui->cbWordCutoff->currentText() == "100k Words" ? 100 : 60;
        lastPart=lastPart.arg(lengthCutoff);
        QString resultString = "https://www.fanfiction.net" + q.value(0).toString() + lastPart;
        result.push_back(resultString);
        qDebug() << result;
    }
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
    QPoint tempPoint(ui->edtResults->x(), 0);
    tempPoint = mapToGlobal(ui->twMain->mapTo(this, tempPoint));
    tagWidgetDynamic->move(tempPoint.x(), pos.y());
    tagWidgetDynamic->setWindowFlags(Qt::FramelessWindowHint);


    tagWidgetDynamic->show();
    tagWidgetDynamic->setFocus();
}

void MainWindow::ReadTags()
{
    QSqlDatabase db = QSqlDatabase::database();
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
    FillRecTagBuildCombobox();
    FillRecTagCombobox();
}

void MainWindow::SetTag(int id, QString tag, bool silent)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("insert into fictags(fic_id, tag) values(:fic_id, :tag)");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag", tag);
    q.bindValue(":fic_id",id);
    q.exec();
    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
        qDebug() << q.lastError();


    if(!silent && !tagList.contains(tag))
    {
        q.prepare("INSERT INTO TAGS(TAG) VALUES(:tag)");
        q.bindValue(":tag", tag);
        q.exec();
        if(q.lastError().isValid())
            qDebug() << q.lastError();
        tagList.push_back(tag);
    }
}

void MainWindow::UnsetTag(int id, QString tag)
{
    QSqlDatabase db = QSqlDatabase::database();

    QString qs = QString("select tags from fanfics where ID = :id");

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


    qs = QString("update fanfics set tags = :tags where ID = :id");

    QSqlQuery q1(db);
    q1.prepare(qs);
    q1.bindValue(":tags", " none " + originaltags.join(" "));
    q1.bindValue(":id", id);
    q1.exec();
    if(q1.lastError().isValid())
        qDebug() << q1.lastError();
}



void MainWindow::PopulateIdList(std::function<QSqlQuery(QString)> bindQuery, QString query, bool forceUpdate)
{
    if(randomIdLists.contains(query) && !forceUpdate)
        return;
    randomIdLists.remove(query);
    randomIdLists.insert(query, QList<int>());
    QString qS = query;

    int posFrom= qS.indexOf("from fanfics f where 1 = 1");
    QString temp = qS.right(qS.length() - posFrom);
    temp = temp.remove(CreateLimitQueryPart());
    qS = "select id " + temp;
    qDebug() << qS;
    QSqlQuery q = bindQuery(qS);
    q.exec();
    while(q.next())
        randomIdLists[query].push_back(q.value("ID").toInt());
}

QString MainWindow::AddIdList(QString query, int count)
{
    QString idList;
    if(!ui->chkRandomizeSelection->isChecked() || !randomIdLists.contains(query) || randomIdLists[query].isEmpty())
        return query;
    std::random_shuffle (randomIdLists[query].begin(), randomIdLists[query].end());
    for(int i(0); i < count && i < randomIdLists[query].size(); i++)
    {
        idList+=QString::number(randomIdLists[query].at(i)) + ",";
    }
    if(idList.isEmpty())
        return idList;
    if(idList.contains(","))
        idList.chop(1);
    idList = "ID IN ( " + idList + " ) ";
    query=query.replace("1 = 1", "1 = 1 AND " + idList + " ");

    return query;
}

QString MainWindow::CreateLimitQueryPart()
{
    QString result;
    int maxFicCountValue = ui->chkFicLimitActivated->isChecked() ? ui->sbMaxFicCount->value()  : 0;
    if(maxFicCountValue > 0 && maxFicCountValue < 51)
        result+= QString(" LIMIT %1 ").arg(QString::number(maxFicCountValue));
    return result;
}

void MainWindow::ProcessRecommendationListsFromDB(QList<core::RecommendationList> list)
{
    lists.clear();
    for(auto value: list)
        lists[value.name] = value;
}

void MainWindow::LoadMoreAuthors(bool reprocessCache)
{
    filter.mode = core::StoryFilter::filtering_in_recommendations;
    //QStringList uniqueAuthors = GetUniqueAuthorsFromActiveRecommenderSet();
    QStringList uniqueAuthors ;
    if(uniqueAuthors.size() == 0)
    {
        auto authors = database::GetAllAuthors("ffn");
        uniqueAuthors.reserve(authors.size());
        for(auto author: authors)
        {
            uniqueAuthors.push_back(author.url("ffn"));
        }
    }
    AddToProgressLog("Authors: " + QString::number(uniqueAuthors.size()));

    ReinitProgressbar(uniqueAuthors.size());
    StartPageWorker();
    auto cacheMode = ui->chkWaveOnlyCache->isChecked() ? ECacheMode::use_only_cache : ECacheMode::use_cache;
    emit pageTaskList(uniqueAuthors, cacheMode);
    DisableAllLoadButtons();
    WebPage webPage;
    auto job = [&](QString url, QString content){
        FavouriteStoryParser parser;
        parser.ProcessPage(url, content);
        return parser;
    };
    QList<QFuture<FavouriteStoryParser>> futures;
    QList<FavouriteStoryParser> parsers;
    An<PageManager> pager;
    int cachedPages = 0;
    int loadedPages = 0;
    QSqlDatabase db = QSqlDatabase::database();
    bool hasTransactions = db.driver()->hasFeature(QSqlDriver::Transactions);
    bool transOpen = db.transaction();
    do
    {
        futures.clear();
        parsers.clear();
        while(pageQueue.isEmpty())
            QCoreApplication::processEvents();
        webPage = pageQueue[0];
        pageQueue.pop_front();
        if(webPage.isFromCache && !reprocessCache)
            continue;

        if(!webPage.isFromCache)
        {
            pager->SavePageToDB(webPage);
            loadedPages++;
        }
        else
            cachedPages++;



        qDebug() << "Page loaded in: " << webPage.loadedIn;
        pbMain->setValue(pbMain->value()+1);
        pbMain->setTextVisible(true);
        pbMain->setFormat("%v");

        auto id = database::GetAuthorIdFromUrl(webPage.url);
        if(id == -1 || reprocessCache)
        {

            auto startRecLoad = std::chrono::high_resolution_clock::now();


            auto splittings = SplitJob(webPage.content);
            //QString  authorName = splittings.authorName;
            if(splittings.storyCountInWhole > 2000)
            {
                InsertLogIntoEditor(ui->edtResults, "Skipping page with too much favourites");
                continue;
            }

            for(auto part: splittings.parts)
            {
                futures.push_back(QtConcurrent::run(job, webPage.url, part.data));
            }
            for(auto future: futures)
            {
                future.waitForFinished();
                parsers+=future.result();
            }

            auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
            qDebug() << "Page Processing done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            qDebug() << "Count of parts:" << parsers.size();

            int sum = 0;
            for(auto actualParser: parsers)
            {
                sum+=actualParser.processedStuff.count() ;
                if(actualParser.processedStuff.size() < 2000)
                {
                    actualParser.WriteProcessed();
                    //actualParser.WriteJustAuthorName();
                }
            }
            InsertLogIntoEditor(ui->edtResults, webPage.url);
            AddToProgressLog(" All Faves: " + QString::number(sum) + " ");

            elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
            qDebug() << "Completed author in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            ui->edtResults->ensureCursorVisible();

        }
    }while(!webPage.isLastPage);
    db.commit();

    //parser.ClearDoneCache();
    ui->edtResults->clear();
    AddToProgressLog(" Pages read from cache: " + QString::number(cachedPages));
    AddToProgressLog(" Pages read from web " + QString::number(loadedPages));
    AddToProgressLog(" Found recommenders: ");
    ShutdownProgressbar();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    EnableAllLoadButtons();
}

void MainWindow::UpdateAllAuthorsWith(std::function<void(core::Author, WebPage)> updater)
{
    filter.mode = core::StoryFilter::filtering_in_recommendations;
    auto authors = database::GetAllAuthors("ffn");
    AddToProgressLog("Authors: " + QString::number(authors.size()));

    ReinitProgressbar(authors.size());
    DisableAllLoadButtons();
    An<PageManager> pager;

    for(auto author: authors)
    {
        auto webPage = pager->GetPage(author.url("ffn"), ECacheMode::use_only_cache);
        qDebug() << "Page loaded in: " << webPage.loadedIn;
        pbMain->setValue(pbMain->value()+1);
        pbMain->setTextVisible(true);
        pbMain->setFormat("%v");
        updater(author, webPage);
    }
    //parser.ClearDoneCache();
    ui->edtResults->clear();
    ShutdownProgressbar();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    EnableAllLoadButtons();
}

void MainWindow::ReprocessAuthors()
{
    auto functor = [](core::Author author, WebPage webPage){
        //auto splittings = SplitJob(webPage.content);
        QString authorName = ParseAuthorNameFromFavouritePage(webPage.content);
        author.name = authorName;
        database::AssignNewNameForRecommenderId(author);
    };
    UpdateAllAuthorsWith(functor);
}

//void MainWindow::ReprocessTagSumRecs()
//{
//    auto tags = database::ReadAvailableRecommendationLists();
//    for(auto tag : tags)
//    {
//        if(tag == "core" || tag == "none")
//            continue;
//        database::UpdateTagStatsPerFic(tag);
//    }
//}

void MainWindow::ProcessListIntoRecommendations(QString list)
{
    QFile data(list);
    QStringList usedList;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
        core::RecommendationList params;
        params.name = in.readLine().split("#").at(1);
        params.minimumMatch= in.readLine().split("#").at(1).toInt();
        params.pickRatio = in.readLine().split("#").at(1).toDouble();
        params.alwaysPickAt = in.readLine().split("#").at(1).toInt();
        database::CreateOrUpdateRecommendationList(params);
        QString str;
        do{
            str = in.readLine();
            QRegExp rx("/s/(\\d+)");
            int pos = rx.indexIn(str);
            QString ficIdPart;
            if(pos != -1)
            {
                ficIdPart = rx.cap(0);
            }
            if(ficIdPart.isEmpty())
                continue;
            //int id = database::GetFicDBIdByDelimitedSiteId(ficIdPart);
            auto webId = core::FFNUrlUtils::GetWebId(ficIdPart);
            if(webId.toInt() <= 0)
                continue;
            int id = database::GetFicIdByWebId(webId.toInt());
            if(id == -1)
                continue;
            qDebug()<< "Settign tag: " << params.name << " to: " << id;
            usedList.push_back(str);
            SetTag(id, "generictag");
        }while(!str.isEmpty());
        params.tagToUse ="generictag";
        BuildRecommendations(params);
        database::DeleteTagfromDatabase("generictag");
        qDebug() << "using list: " << usedList;
    }
}

void MainWindow::ProcessTagIntoRecommenders(QString tag)
{
    if(!recommendersModel)
        return;
    QStringList result;
    auto allStats = database::GetRecommenderStatsForList(tag, "(1/match_ratio)*match_count", "desc");
    for(auto stat : allStats)
        result.push_back(stat.authorName);
    recommendersModel->setStringList(result);
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

QStringList MainWindow::GetCurrentFilterUrls(QString selectedFandom, bool crossoverState, bool ignoreTrackingState)
{
    QStringList urls;
    if(crossoverState)
    {
        urls = GetCrossoverUrl(selectedFandom,ignoreTrackingState);
        lastUpdated = database::GetMaxUpdateDateForFandom(QStringList() << selectedFandom << "CROSSOVER");
        isCrossover = true;
        currentFandom = selectedFandom;
    }
    else
    {

        isCrossover = false;
        urls = GetNormalUrl(selectedFandom,ignoreTrackingState);
        lastUpdated = database::GetMaxUpdateDateForFandom(QStringList() << selectedFandom);
        currentFandom = selectedFandom;

    }

    qDebug() << "got last update date";
    return urls;
}

void MainWindow::DisableAllLoadButtons()
{
    ui->pbCrawl->setEnabled(false);
    ui->pbFirstWave->setEnabled(false);
    ui->pbLoadTrackedFandoms->setEnabled(false);
    ui->pbLoadPage->setEnabled(false);
    ui->pbLoadAllRecommenders->setEnabled(false);
}
void MainWindow::EnableAllLoadButtons()
{
    ui->pbCrawl->setEnabled(true);
    ui->pbFirstWave->setEnabled(true);
    ui->pbLoadTrackedFandoms->setEnabled(true);
    ui->pbLoadPage->setEnabled(true);
    ui->pbLoadAllRecommenders->setEnabled(true);
}

void MainWindow::WipeSelectedFandom(bool)
{
    QString fandom;
    if(ui->rbCrossovers->isChecked())
    {
        fandom = ui->cbNormals->currentText();
    }
    else
    {
        fandom = ui->cbNormals->currentText();
    }
    if(!fandom.isEmpty())
    {
        QSqlDatabase db = QSqlDatabase::database();
        QString qs = QString("delete from fanfics where fandom like '%%1%'");
        qs=qs.arg(fandom);
        QSqlQuery q(qs, db);
        q.exec();
    }
}

void MainWindow::OnNewPage(WebPage page)
{
    pageQueue.push_back(page);
}

void MainWindow::OnCopyFicUrl(QString text)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
    ui->edtResults->insertPlainText(text + "\n");

}

void MainWindow::OnCopyAllUrls()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString result;
    for(int i = 0; i < typetableModel->rowCount(); i ++)
    {
        if(ui->chkInfoForLinks->isChecked())
        {
            result += typetableModel->index(i, 2).data().toString() + "\n";
        }
        result += "http://www.fanfiction.net" + typetableModel->index(i, 9).data().toString() + "\n\n";
    }
    clipboard->setText(result);
}


bool MainWindow::CheckSectionAvailability()
{
    QSqlDatabase db = QSqlDatabase::database();
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
    ui->chkShowDirectRecs->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->wdgWave->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->cbRecTagGroup->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->pbFirstWave->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->pbWipeFandom->setVisible(settings.value("Settings/pbWipeFandom", false).toBool());

    ui->cbNormals->setCurrentText(settings.value("Settings/normals", "").toString());

    ui->chkTrackedFandom->blockSignals(true);
    ui->chkTrackedFandom->setChecked(database::FetchTrackStateForFandom(ui->cbNormals->currentText(),ui->rbCrossovers->isChecked()));
    ui->chkTrackedFandom->blockSignals(false);

    ui->cbMaxWordCount->setCurrentText(settings.value("Settings/maxWordCount", "").toString());
    ui->cbMinWordCount->setCurrentText(settings.value("Settings/minWordCount", 100000).toString());

    ui->leContainsGenre->setText(settings.value("Settings/plusGenre", "").toString());
    ui->leNotContainsGenre->setText(settings.value("Settings/minusGenre", "").toString());
    ui->leNotContainsWords->setText(settings.value("Settings/minusWords", "").toString());
    ui->leContainsWords->setText(settings.value("Settings/plusWords", "").toString());

    ui->chkGenrePlus->setChecked(settings.value("Settings/chkGenrePlus", false).toBool());
    ui->chkGenreMinus->setChecked(settings.value("Settings/chkGenreMinus", false).toBool());
    ui->chkWordsPlus->setChecked(settings.value("Settings/chkWordsPlus", false).toBool());
    ui->chkWordsMinus->setChecked(settings.value("Settings/chkWordsMinus", false).toBool());

    ui->chkActive->setChecked(settings.value("Settings/active", false).toBool());
    ui->chkShowUnfinished->setChecked(settings.value("Settings/showUnfinished", false).toBool());
    ui->chkNoGenre->setChecked(settings.value("Settings/chkNoGenre", false).toBool());
    ui->chkCacheMode->setChecked(settings.value("Settings/cacheMode", false).toBool());
    ui->chkComplete->setChecked(settings.value("Settings/completed", false).toBool());
    ui->gbTagFilters->setChecked(settings.value("Settings/filterOnTags", false).toBool());
    ui->spMain->restoreState(settings.value("Settings/spMain", false).toByteArray());
    ui->spDebug->restoreState(settings.value("Settings/spDebug", false).toByteArray());
    ui->cbSortMode->blockSignals(true);
    ui->cbCustomFilters->blockSignals(true);
    ui->chkCustomFilter->blockSignals(true);
    ui->leAuthorUrl->setText(settings.value("Settings/currentRecommender", "").toString());
    ui->chkShowRecsRegardlessOfTags->setChecked(settings.value("Settings/ignoreTagsOnRecommendations", false).toBool());
    ui->cbSortMode->setCurrentText(settings.value("Settings/currentSortFilter", "Update Date").toString());
    ui->cbCustomFilters->setCurrentText(settings.value("Settings/currentSortFilter", "Longest Running").toString());
    ui->cbWordCutoff->setCurrentText(settings.value("Settings/lengthCutoff", "100k Words").toString());
    ui->chkCustomFilter->setChecked(settings.value("Settings/customFilterEnabled", false).toBool());
    ui->cbSortMode->blockSignals(false);
    ui->cbCustomFilters->blockSignals(false);
    ui->chkCustomFilter->blockSignals(false);
    ui->cbBiasFavor->setCurrentText(settings.value("Settings/biasMode", "None").toString());
    ui->cbBiasOperator->setCurrentText(settings.value("Settings/biasOperator", "<").toString());
    ui->leBiasValue->setText(settings.value("Settings/biasValue", "2.5").toString());

}

void MainWindow::WriteSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.setValue("Settings/minWordCount", ui->cbMinWordCount->currentText());
    settings.setValue("Settings/maxWordCount", ui->cbMaxWordCount->currentText());
    settings.setValue("Settings/normals", ui->cbNormals->currentText());
    //settings.setValue("Settings/crossovers", ui->cbCrossovers->currentText());
    settings.setValue("Settings/plusGenre", ui->leContainsGenre->text());
    settings.setValue("Settings/minusGenre", ui->leNotContainsGenre->text());
    settings.setValue("Settings/plusWords", ui->leContainsWords->text());
    settings.setValue("Settings/minusWords", ui->leNotContainsWords->text());
    settings.setValue("Settings/section", ui->cbSectionTypes->currentText());


    settings.setValue("Settings/chkGenrePlus", ui->chkGenrePlus->isChecked());
    settings.setValue("Settings/chkGenreMinus", ui->chkGenreMinus->isChecked());
    settings.setValue("Settings/chkWordsPlus", ui->chkWordsPlus->isChecked());
    settings.setValue("Settings/chkWordsMinus", ui->chkWordsMinus->isChecked());

    settings.setValue("Settings/active", ui->chkActive->isChecked());
    settings.setValue("Settings/showUnfinished", ui->chkShowUnfinished->isChecked());
    settings.setValue("Settings/chkNoGenre", ui->chkNoGenre->isChecked());
    settings.setValue("Settings/cacheMode", ui->chkCacheMode->isChecked());
    settings.setValue("Settings/completed", ui->chkComplete->isChecked());
    settings.setValue("Settings/filterOnTags", ui->gbTagFilters->isChecked());
    settings.setValue("Settings/spMain", ui->spMain->saveState());
    settings.setValue("Settings/spDebug", ui->spDebug->saveState());
    settings.setValue("Settings/currentSortFilter", ui->cbSortMode->currentText());
    settings.setValue("Settings/currentCustomFilter", ui->cbCustomFilters->currentText());
    settings.setValue("Settings/currentRecommender", ui->leAuthorUrl->text());
    settings.setValue("Settings/ignoreTagsOnRecommendations", ui->chkShowRecsRegardlessOfTags->isChecked());
    settings.setValue("Settings/customFilterEnabled", ui->chkCustomFilter->isChecked());
    settings.setValue("Settings/biasMode", ui->cbBiasFavor->currentText());
    settings.setValue("Settings/biasOperator", ui->cbBiasOperator->currentText());
    settings.setValue("Settings/biasValue", ui->leBiasValue->text());
    settings.setValue("Settings/lengthCutoff", ui->cbWordCutoff->currentText());
    settings.sync();
}

void MainWindow::ProcessFandoms(WebPage webPage)
{

    QString str(webPage.content);
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
        names.insert({currentProcessedSection,name}, core::Fandom{name, currentProcessedSection, link, ""});
    }
    managerEventLoop.quit();
}

void MainWindow::ProcessCrossovers(WebPage webPage)
{
    QString str(webPage.content);
    //QString pattern = sections[currentProcessedSection].section;//.mid(indexOfSlash + 1, currentProcessedSection.length() - (indexOfSlash +1)) + "\\sCrossovers";
    QRegExp rxStartFandoms("<TABLE\\sWIDTH='100%'><TR>");
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
        QRegExp rxStartLink("href=[\"]");
        QRegExp rxEndLink("/\"");

        int linkStart = rxStartLink.indexIn(str, indexStart);
        if(linkStart == -1)
            break;
        int linkEnd= rxEndLink.indexIn(str, linkStart);
        if(linkStart == -1 || linkEnd == -1)
        {
            QMessageBox::warning(0, "warning!", "failed to fetch link at: ", str.mid(linkStart, str.size() - linkStart));
        }
        QString link = str.mid(linkStart + rxStartLink.pattern().length()-2,
                               linkEnd - (linkStart + rxStartLink.pattern().length())+2);

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

        indexStart = linkEnd;
        if(!names.contains({currentProcessedSection,name}))
            names.insert({currentProcessedSection,name},core::Fandom{name, currentProcessedSection,  "",link});
        else
            names[{currentProcessedSection,name}].crossoverUrl = link;
    }
    managerEventLoop.quit();
}

void MainWindow::OnChapterUpdated(QVariant chapter, QVariant author, QVariant title)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set at_chapter = :chapter where author = :author and title = :title");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":chapter", chapter.toInt());
    q.bindValue(":author", author.toString());
    q.bindValue(":title", title.toString());
    q.exec();
    qDebug() << chapter.toInt() << " " << author.toString() << " " <<  title.toString();
    if(q.lastError().isValid())
        qDebug() << q.lastError();

}

void MainWindow::OnTagAdd(QVariant tag, QVariant row)
{
    int rownum = row.toInt();
    SetTag(rownum, tag.toString());
    QModelIndex index = typetableModel->index(rownum, 10);
    auto data = typetableModel->data(index, 0).toString();
    data += " " + tag.toString();
    typetableModel->setData(index,data,0);
    typetableModel->updateAll();
}

void MainWindow::OnTagRemove(QVariant tag, QVariant row)
{
    UnsetTag(row.toInt(), tag.toString());
    QModelIndex index = typetableModel->index(row.toInt(), 10);
    auto data = typetableModel->data(index, 0).toString();
    data = data.replace(tag.toString(), "");
    typetableModel->setData(index,data,0);
    typetableModel->updateAll();
}

void MainWindow::OnTagClicked(QVariant tag, QVariant currentMode, QVariant row)
{
}

void MainWindow::on_pbCrawl_clicked()
{
    processedFics = 0;
    currentFilterUrls = GetCurrentFilterUrls(ui->cbNormals->currentText(), ui->rbCrossovers->isChecked(), true);
    pageCounter = 0;
    ui->edtResults->clear();
    processedCount = 0;
    ignoreUpdateDate = false;
    nextUrl = QString();
    for(QString url: currentFilterUrls)
    {
        currentFilterUrl = url;
        QString crossOverAddin = ui->rbCrossovers->isChecked() ? "CROSSOVER" : "";
        auto lastUpdated = database::GetMaxUpdateDateForFandom(QStringList() << ui->cbNormals->currentText() << crossOverAddin);
        RequestAndProcessPage(ui->cbNormals->currentText(), lastUpdated, url);
    }
    QMessageBox::information(nullptr, "Info", QString("finished processing %1 fics" ).arg(processedFics));
    database::PushFandom(ui->cbNormals->currentText().trimmed());
    recentFandomsModel->setStringList(database::FetchRecentFandoms());
    ui->lvTrackedFandoms->setModel(recentFandomsModel);

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

void MainWindow::OnCustomFilterClicked()
{
    if(ui->chkCustomFilter->isChecked())
    {
        ui->cbCustomFilters->setEnabled(true);
        QPalette p = ui->cbCustomFilters->palette();
        ui->chkCustomFilter->setStyleSheet("QCheckBox {border: none; color: DarkGreen;}");
        ui->cbCustomFilters->setPalette(p);
    }
    else
    {
        ui->cbCustomFilters->setEnabled(false);
        QPalette p = ui->cbSortMode->palette();
        ui->cbCustomFilters->setPalette(p);
        ui->chkCustomFilter->setStyleSheet("");
    }
    //on_pbLoadDatabase_clicked();
}

void MainWindow::OnSectionReloadActivated()
{
    ui->pbInit->setEnabled(ui->chkActivateReloadSectionData->isChecked());
}


void MainWindow::OnShowContextMenu(QPoint p)
{
    browserMenu.popup(this->mapToGlobal(p));
}

void MainWindow::OnSectionChanged(QString)
{
    ui->cbNormals->setModel(new QStringListModel(database::GetFandomListFromDB(ui->cbSectionTypes->currentText())));
}

void MainWindow::on_pbLoadDatabase_clicked()
{
    filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_fics);
    LoadData();

    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
}

void MainWindow::on_pbInit_clicked()
{
    Init();
}


void MainWindow::OnCheckboxFilter(int)
{
    LoadData();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
    typetableModel->OnReloadDataFromInterface();
}

void MainWindow::on_chkRandomizeSelection_clicked(bool checked)
{
    int maxFicCountValue = ui->chkFicLimitActivated->isChecked() ? ui->sbMaxFicCount->value()  : 0;
    if(checked && maxFicCountValue < 1 || maxFicCountValue >50)
        ui->sbMaxFicCount->setValue(10);
}

void MainWindow::on_cbCustomFilters_currentTextChanged(const QString &arg1)
{
    on_pbLoadDatabase_clicked();
}

void MainWindow::on_cbSortMode_currentTextChanged(const QString &arg1)
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(ui->cbSortMode->currentText() == "Rec Count")
        ui->cbRecGroup->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
}



void MainWindow::on_pbExpandPlusGenre_clicked()
{
    currentExpandedEdit = ui->leContainsGenre;
    CallExpandedWidget();
}

void MainWindow::on_pbExpandMinusGenre_clicked()
{
    currentExpandedEdit = ui->leNotContainsGenre;
    CallExpandedWidget();
}

void MainWindow::on_pbExpandPlusWords_clicked()
{
    currentExpandedEdit = ui->leContainsWords;
    CallExpandedWidget();
}

void MainWindow::on_pbExpandMinusWords_clicked()
{
    currentExpandedEdit = ui->leNotContainsWords;
    CallExpandedWidget();
}

void MainWindow::OnNewSelectionInRecentList(const QModelIndex &current, const QModelIndex &previous)
{
    ui->cbNormals->setCurrentText(current.data().toString());
    ui->chkTrackedFandom->blockSignals(true);
    ui->chkTrackedFandom->setChecked(database::FetchTrackStateForFandom(ui->cbNormals->currentText(),ui->rbCrossovers->isChecked()));
    ui->chkTrackedFandom->blockSignals(false);
}

void MainWindow::OnNewSelectionInRecommenderList(const QModelIndex &current, const QModelIndex &previous)
{
    QString recommender = current.data().toString();
    if(recommenders.contains(recommender))
        ui->leAuthorUrl->setText(recommenders[recommender].url("ffn"));
}

void MainWindow::CallExpandedWidget()
{
    if(!expanderWidget)
    {
        expanderWidget = new QDialog();
        expanderWidget->resize(400, 300);
        QVBoxLayout* vl = new QVBoxLayout;
        QPushButton* okButton = new QPushButton;
        okButton->setText("OK");
        edtExpander = new QTextEdit;
        vl->addWidget(edtExpander);
        vl->addWidget(okButton);
        expanderWidget->setLayout(vl);
        connect(okButton, &QPushButton::clicked, [&](){
            if(currentExpandedEdit)
                currentExpandedEdit->setText(edtExpander->toPlainText());
            expanderWidget->hide();
        });
    }
    if(currentExpandedEdit)
        edtExpander->setText(currentExpandedEdit->text());
    expanderWidget->exec();
}

QStringList MainWindow::SortedList(QStringList list)
{
    qSort(list.begin(),list.end());
    return list;
}
QStringList MainWindow::ReverseSortedList(QStringList list)
{
    qSort(list.begin(),list.end());
    std::reverse(list.begin(),list.end());
    return list;
}


QStringList MainWindow::GetUniqueAuthorsFromActiveRecommenderSet()
{
    QStringList result;
    QList<core::Fic> sections;
    int counter = 0;
    auto list = ReverseSortedList(recommenders.keys());
    auto job = [](QString url, QString content){
        QList<core::Fic> sections;
        FavouriteStoryParser parser;
        sections += parser.ProcessPage(url, content);
        return sections;
    };
    QList<QFuture<QList<core::Fic>>> futures;
    An<PageManager> pager;
    QHash<QString, core::Fic> uniqueSections;
    pager->SetDatabase(QSqlDatabase::database());
    for(auto recName: list)
    {

        auto startRecProcessing = std::chrono::high_resolution_clock::now();
        counter++;
        //        if(counter != 4)
        //            continue;
        core::Author recommender = recommenders[recName];
        uniqueSections[recommender.url("ffn")] = core::Fic();

        InsertLogIntoEditor(ui->edtResults, recommender.url("ffn"));
        auto page = pager->GetPage(recommender.url("ffn"), ECacheMode::use_cache);
        if(!page.isFromCache)
            pager->SavePageToDB(page);


        auto splittings = SplitJob(page.content);
        for(auto part: splittings.parts)
        {
            futures.push_back(QtConcurrent::run(job, page.url, part.data));
        }
        for(auto future: futures)
        {
            future.waitForFinished();
            sections+=future.result();
        }
        if(!page.isFromCache)
        {
            qDebug() << "Sleeping";
            QThread::sleep(1);
        }
        auto elapsed = std::chrono::high_resolution_clock::now() - startRecProcessing;
        qDebug() << "Recommender processing done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    }


    for(auto section: sections)
        uniqueSections[section.author.url("ffn")] = section;
    result = SortedList(uniqueSections.keys());
    return result;
}


void MainWindow::CreatePageThreadWorker()
{
    worker = new PageThreadWorker;
    worker->moveToThread(&pageThread);
    connect(this, &MainWindow::pageTask, worker, &PageThreadWorker::Task);
    connect(this, &MainWindow::pageTaskList, worker, &PageThreadWorker::TaskList);
    connect(worker, &PageThreadWorker::pageReady, this, &MainWindow::OnNewPage);
}

void MainWindow::StartPageWorker()
{
    pageQueue.clear();
    pageThread.start(QThread::HighPriority);

}

void MainWindow::StopPageWorker()
{
    pageThread.quit();
}

void MainWindow::ReinitProgressbar(int maxValue)
{
    pbMain->setMaximum(maxValue);
    pbMain->setValue(0);
    pbMain->show();
}

void MainWindow::ShutdownProgressbar()
{
    pbMain->setValue(0);
    pbMain->hide();
}

void MainWindow::AddToProgressLog(QString value)
{
    ui->edtResults->insertHtml(value);
}

void MainWindow::FillRecTagBuildCombobox()
{
    //ProcessRecommendationListsFromDB(database::GetAvailableRecommendationLists());
    ui->cbRecTagBuildGroup->setModel(new QStringListModel(tagList));

}

void MainWindow::FillRecTagCombobox()
{
    //ProcessRecommendationListsFromDB(database::GetAvailableRecommendationLists());
    ui->cbRecGroup->setModel(new QStringListModel(lists.keys()));
    ui->cbRecTagGroup->setModel(new QStringListModel(lists.keys()));
}


void MainWindow::on_chkTrackedFandom_toggled(bool checked)
{
    database::SetFandomTracked( ui->cbNormals->currentText(),ui->rbCrossovers->isChecked(), checked);
}

void MainWindow::on_rbNormal_clicked()
{
    ui->chkTrackedFandom->blockSignals(true);
    ui->chkTrackedFandom->setChecked(database::FetchTrackStateForFandom(ui->cbNormals->currentText(),ui->rbCrossovers->isChecked()));
    ui->chkTrackedFandom->blockSignals(false);
}

void MainWindow::on_rbCrossovers_clicked()
{
    ui->chkTrackedFandom->blockSignals(true);
    ui->chkTrackedFandom->setChecked(database::FetchTrackStateForFandom(ui->cbNormals->currentText(),ui->rbCrossovers->isChecked()));
    ui->chkTrackedFandom->blockSignals(false);
}

void MainWindow::on_pbLoadTrackedFandoms_clicked()
{
    processedFics = 0;
    auto trackedFandoms = database::FetchTrackedFandoms();
    qDebug() << trackedFandoms;
    for(QString fandom : trackedFandoms)
    {
        currentFilterUrls = GetCurrentFilterUrls(fandom, false);
        pageCounter = 0;
        ui->edtResults->clear();
        processedCount = 0;
        ignoreUpdateDate = false;
        nextUrl = QString();
        for(QString url: currentFilterUrls)
        {
            currentFilterUrl = url;
            RequestAndProcessPage(fandom, lastUpdated, url);
        }
    }
    auto trackedCrossovers = database::FetchTrackedCrossovers();
    qDebug() << trackedCrossovers;
    for(QString fandom : database::FetchTrackedCrossovers())
    {
        currentFilterUrls = GetCurrentFilterUrls(fandom, true);
        pageCounter = 0;
        ui->edtResults->clear();
        processedCount = 0;
        ignoreUpdateDate = false;
        nextUrl = QString();
        for(QString url: currentFilterUrls)
        {
            currentFilterUrl = url;
            RequestAndProcessPage(fandom,lastUpdated, url);
        }
    }
    QMessageBox::information(nullptr, "Info", QString("finished processing %1 fics" ).arg(processedFics));
    //ui->sb
}

void MainWindow::on_pbLoadPage_clicked()
{
    filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations);
    auto startPageRequest = std::chrono::high_resolution_clock::now();
    auto page = RequestPage(ui->leAuthorUrl->text(),  ui->chkWaveOnlyCache->isChecked() ? ECacheMode::use_only_cache : ECacheMode::use_cache);
    auto elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
    qDebug() << "Fetched page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    FavouriteStoryParser parser;
    auto startPageProcess = std::chrono::high_resolution_clock::now();
    QString name = ParseAuthorNameFromFavouritePage(page.content);
    parser.authorName = name;
    parser.ProcessPage(page.url, page.content);
    elapsed = std::chrono::high_resolution_clock::now() - startPageProcess;
    qDebug() << "Processed page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->clear();
    ui->edtResults->insertHtml(parser.diagnostics.join(""));
    auto startRecLoad = std::chrono::high_resolution_clock::now();

    currentRecommenderId = database::GetAuthorIdFromUrl(page.url);
    if(currentRecommenderId == -1)
    {
        database::WriteRecommender(parser.recommender.author);
        currentRecommenderId = database::GetAuthorIdFromUrl(page.url);
    }

    parser.WriteProcessed();

    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));

    LoadData();
    elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);

}

void MainWindow::on_pbRemoveRecommender_clicked()
{
    QString currentSelection = ui->lvRecommenders->selectionModel()->currentIndex().data().toString();
    if(recommenders.contains(currentSelection))
    {
        database::RemoveAuthor(recommenders[currentSelection]);
        recommenders = database::FetchRecommenders();
        recommendersModel->setStringList(SortedList(recommenders.keys()));
    }
}

void MainWindow::on_pbOpenRecommendations_clicked()
{
    filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations);

    auto startRecLoad = std::chrono::high_resolution_clock::now();

    currentRecommenderId = database::GetAuthorIdFromUrl(ui->leAuthorUrl->text());

    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));
    LoadData();

    auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
}

void MainWindow::on_pbLoadAllRecommenders_clicked()
{
    filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations);

    auto startPageRequest = std::chrono::high_resolution_clock::now();
    for(auto recommender: recommenders.values())
    {
        auto page = RequestPage(recommender.url("ffn"), ui->chkWaveOnlyCache->isChecked() ? ECacheMode::use_only_cache : ECacheMode::use_cache);
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
        qDebug() << "Fetched page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        FavouriteStoryParser parser;
        auto startPageProcess = std::chrono::high_resolution_clock::now();
        parser.ProcessPage(page.url, QString(page.content));
        parser.WriteProcessed();
        elapsed = std::chrono::high_resolution_clock::now() - startPageProcess;
        qDebug() << "Processed page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->clear();
        ui->edtResults->insertHtml(parser.diagnostics.join(""));

    }
    ui->leAuthorUrl->setText("");
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    currentRecommenderId = database::GetAuthorIdFromUrl(ui->leAuthorUrl->text());

    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));

    LoadData();
    auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
}

void MainWindow::on_pbOpenWholeList_clicked()
{
    filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations);

    ui->leAuthorUrl->setText("");
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    currentRecommenderId = database::GetAuthorIdFromUrl(ui->leAuthorUrl->text());
    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));

    LoadData();
    auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
}

void MainWindow::on_pbFirstWave_clicked()
{
    //!!! bool reprocessCache = !database::HasNoneTagInRecommendations();

    LoadMoreAuthors(true);
}

void MainWindow::OnReloadRecLists()
{
    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));
}

void MainWindow::on_cbUseDateCutoff_clicked()
{
    ui->deCutoffLimit->setEnabled(!ui->deCutoffLimit->isEnabled());
}


void MainWindow::BuildRecommendations(core::RecommendationList params)
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QList<int> fullRecommenderList = database::GetFulLRecommenderList();
    std::sort(std::begin(fullRecommenderList),std::end(fullRecommenderList));
    QList<int> resultingRecommenderList;
    resultingRecommenderList.reserve(fullRecommenderList.size()/10);
    database::DeleteRecommendationList(params.name);
    database::CreateOrUpdateRecommendationList(params);
    for(auto id: fullRecommenderList)
    {
        auto stats = database::CreateRecommenderStats(id, params);

        if( stats.matchesWithReferenceTag >= params.alwaysPickAt
                || (stats.matchRatio <= params.pickRatio && stats.matchesWithReferenceTag >= params.minimumMatch) )
        {
            database::CopyAllAuthorRecommendationsToList(id,params.name);
            database::IncrementAllValuesInListMatchingAuthorFavourites(id,params.name);
            database::WriteRecommenderStatsForTag(stats, params.id);
            resultingRecommenderList.push_back(id);
        }
    }
    database::UpdateFicCountForRecommendationList(params);
    lists[params.name] = params;

    if(resultingRecommenderList.size() > 0)
    {
        FillRecTagCombobox();
        QStringList result;
        auto allStats = database::GetRecommenderStatsForList(params.name, "(1/match_ratio)*match_count", "desc");
        for(auto stat : allStats)
            result.push_back(stat.authorName);
        recommendersModel->setStringList(result);
    }
    db.commit();
}

core::StoryFilter MainWindow::ProcessGUIIntoStoryFilter(core::StoryFilter::EFilterMode mode)
{
    auto valueIfChecked = [](QCheckBox* box, auto value){
        if(box->isChecked())
            return value;
        return decltype(value)();
    };
    core::StoryFilter filter;
    filter.activeTags = ui->wdgTagsPlaceholder->GetSelectedTags();
    qDebug() << filter.activeTags;
    filter.allowNoGenre = ui->chkNoGenre->isChecked();
    filter.allowUnfinished = ui->chkShowUnfinished->isChecked();
    filter.ensureActive = ui->chkActive->isChecked();
    filter.ensureCompleted= ui->chkComplete->isChecked();
    filter.fandom = ui->cbNormals->currentText();
    filter.ficCategory = ui->cbSectionTypes->currentText();
    filter.genreExclusion = valueIfChecked(ui->chkGenreMinus, core::StoryFilter::ProcessDelimited(ui->leNotContainsGenre->text(), "###"));
    filter.genreInclusion = valueIfChecked(ui->chkGenrePlus,core::StoryFilter::ProcessDelimited(ui->leContainsGenre->text(), "###"));
    filter.wordExclusion = valueIfChecked(ui->chkWordsMinus, core::StoryFilter::ProcessDelimited(ui->leNotContainsWords->text(), "###"));
    filter.wordInclusion = valueIfChecked(ui->chkWordsPlus, core::StoryFilter::ProcessDelimited(ui->leContainsWords->text(), "###"));
    filter.ignoreAlreadyTagged = ui->chkIgnoreTags->isChecked();
    filter.includeCrossovers = ui->rbCrossovers->isChecked();
    filter.maxFics = valueIfChecked(ui->chkFicLimitActivated, ui->sbMaxFicCount->value());
    filter.minFavourites = valueIfChecked(ui->chkFaveLimitActivated, ui->sbMinimumFavourites->value());
    filter.maxWords= ui->cbMaxWordCount->currentText().toInt();
    filter.minWords= ui->cbMinWordCount->currentText().toInt();
    filter.randomizeResults = ui->chkRandomizeSelection->isChecked();
    filter.recentAndPopularFavRatio = ui->sbFavrateValue->value();
    filter.recentCutoff = ui->dteFavRateCut->dateTime();
    filter.reviewBias = static_cast<core::StoryFilter::EReviewBiasMode>(ui->cbBiasFavor->currentIndex());
    filter.biasOperator = static_cast<core::StoryFilter::EBiasOperator>(ui->cbBiasOperator->currentIndex());
    filter.reviewBiasRatio = ui->leBiasValue->text().toDouble();
    filter.sortMode = static_cast<core::StoryFilter::ESortMode>(ui->cbSortMode->currentIndex());
    filter.listForRecommendations = database::GetRecommendationListIdForName(ui->cbRecGroup->currentText());
    //filter.titleInclusion = nothing for now
    filter.website = "ffn"; // just ffn for now
    filter.mode = mode;
    filter.useThisRecommenderOnly = database::GetAuthorIdFromUrl(ui->leAuthorUrl->text());;
    return filter;
}

void MainWindow::on_pbBuildRecs_clicked()
{
    core::RecommendationList params;
    params.name = ui->cbRecListNames->currentText();
    params.tagToUse = ui->cbRecTagBuildGroup->currentText();
    params.minimumMatch = ui->sbMinRecMatch->value();
    params.pickRatio = ui->dsbMinRecThreshhold->value();
    params.alwaysPickAt = ui->sbAlwaysPickRecAt->value();
    BuildRecommendations(params);
    ProcessRecommendationListsFromDB(database::GetAvailableRecommendationLists());
}

void MainWindow::on_cbRecTagGroup_currentIndexChanged(const QString &tag)
{
    ProcessTagIntoRecommenders(tag);    ;
}

void MainWindow::on_pbOpenAuthorUrl_clicked()
{
    QDesktopServices::openUrl(ui->leAuthorUrl->text());
}

void MainWindow::on_pbReprocessAuthors_clicked()
{
    //ReprocessTagSumRecs();
    ProcessListIntoRecommendations("lists/sneaky.txt");
}

void MainWindow::on_cbRecTagBuildGroup_currentTextChanged(const QString &newText)
{
    if(lists.contains(newText))
    {
        ui->sbMinRecMatch->setValue(lists[newText].minimumMatch);
        ui->dsbMinRecThreshhold->setValue(lists[newText].pickRatio);
        ui->sbAlwaysPickRecAt->setValue(lists[newText].alwaysPickAt);
    }
    else
    {
        ui->sbMinRecMatch->setValue(1);
        ui->dsbMinRecThreshhold->setValue(150);
        ui->sbAlwaysPickRecAt->setValue(1);
        ui->cbRecListNames->setCurrentText("lupine");
    }
}
