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
#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QThread>
#include <chrono>

#include "genericeventfilter.h"

#include <algorithm>
#include "include/init_database.h"
#include "include/favparser.h"

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
    this->setWindowTitle("ffnet sane search engine");
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(settings.value("Settings/hideCache", true).toBool())
        ui->chkCacheMode->setVisible(false);
    ReadTags();
    recentFandomsModel = new QStringListModel;
    recommendersModel= new QStringListModel;
    connect(
                &manager, SIGNAL (finished(QNetworkReply*)),
                this, SLOT (OnNetworkReply(QNetworkReply*)));


    ui->edtResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->edtResults, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnShowContextMenu(QPoint)));
    connect(&fandomManager, &QNetworkAccessManager::finished, this, &MainWindow::OnFandomReply);
    connect(&crossoverManager, &QNetworkAccessManager::finished, this, &MainWindow::OnCrossoverReply);
    connect(ui->cbSectionTypes, SIGNAL(currentTextChanged(QString)), this, SLOT(OnSectionChanged(QString)));
    connect(ui->pbWipeFandom, SIGNAL(clicked(bool)), this, SLOT(WipeSelectedFandom(bool)));

    sections.insert("Anime/Manga", Fandom{"Anime/Manga", "anime", NameOfFandomSectionToLink("anime"), NameOfCrossoverSectionToLink("anime")});
    sections.insert("Misc", Fandom{"Misc", "misc", NameOfFandomSectionToLink("misc"), NameOfCrossoverSectionToLink("misc")});
    sections.insert("Books", Fandom{"Books", "book", NameOfFandomSectionToLink("book"), NameOfCrossoverSectionToLink("book")});
    sections.insert("Movies", Fandom{"Movies", "movie", NameOfFandomSectionToLink("movie"), NameOfCrossoverSectionToLink("movie")});
    sections.insert("Cartoons", Fandom{"Cartoons", "cartoon", NameOfFandomSectionToLink("cartoon"), NameOfCrossoverSectionToLink("cartoon")});
    sections.insert("Comics", Fandom{"Comics", "comic", NameOfFandomSectionToLink("comic"), NameOfCrossoverSectionToLink("comic")});
    sections.insert("Games", Fandom{"Games", "game", NameOfFandomSectionToLink("game"), NameOfCrossoverSectionToLink("game")});
    sections.insert("Plays/Musicals", Fandom{"Plays/Musicals", "play", NameOfFandomSectionToLink("play"), NameOfCrossoverSectionToLink("play")});
    sections.insert("TV Shows", Fandom{"TV Shows", "tv", NameOfFandomSectionToLink("tv"), NameOfCrossoverSectionToLink("tv")});

    //connect(ui->buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnCheckboxFilter(int)));

    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    //ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));

    pbMain = new QProgressBar;
    pbMain->setMaximumWidth(200);
    lblCurrentOperation = new QLabel;
    lblCurrentOperation->setMaximumWidth(300);

    ui->statusBar->addPermanentWidget(lblCurrentOperation,1);
    ui->statusBar->addPermanentWidget(pbMain,0);

    ui->edtResults->setOpenLinks(false);
    connect(ui->edtResults, &QTextBrowser::anchorClicked, this, &MainWindow::OnLinkClicked);

    //    tagEditor->setOpenLinks(false);
    //    tagEditor->setFont(QFont("Verdana", 16));
    //    connect(tagEditor, &QTextBrowser::anchorClicked, this, OnTagClicked);

    // should refer to new tag widget instead
    GenericEventFilter* eventFilter = new GenericEventFilter(this);
    eventFilter->SetEventProcessor(std::bind(TagEditorHider,std::placeholders::_1, std::placeholders::_2, tagWidgetDynamic));
    tagWidgetDynamic->installEventFilter(eventFilter);
    connect(tagWidgetDynamic, &TagWidget::tagToggled, this, &MainWindow::OnTagToggled);

    connect(ui->wdgTagsPlaceholder, &TagWidget::refilter, [&](){
        qwFics->rootContext()->setContextProperty("ficModel", nullptr);
        if(ui->gbTagFilters->isChecked() && ui->wdgTagsPlaceholder->GetSelectedTags().size() > 0)
            LoadData();
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
            QSqlDatabase db = QSqlDatabase::database(dbName);

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
            QSqlDatabase db = QSqlDatabase::database(dbName);

            QSqlQuery q(db);
            q.prepare("INSERT INTO TAGS(TAG) VALUES(:tag)");
            q.bindValue(":tag", tag);
            q.exec();
            if(q.lastError().isValid())
                qDebug() << q.lastError();
            tagList.append(tag);
            qwFics->rootContext()->setContextProperty("tagModel", tagList);
        }

    });

    //ui->wdgTagsPlaceholder->SetAddDialogVisibility(false);
    //    QObject::connect(this, &MainWindow::activated, this, [&](QWidget*){
    //        tagWidgetDynamic->hide();
    //    });
    this->setAttribute(Qt::WA_QuitOnClose);
    ReadSettings();
    SetupFanficTable();
    //ui->tvFanfics->hide();
    IntiConnections();
    database::RebaseFandoms();

    recentFandomsModel->setStringList(database::FetchRecentFandoms());
    ui->lvTrackedFandoms->setModel(recentFandomsModel);
    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));
    ui->lvRecommenders->setModel(recommendersModel);
    connect(ui->lvTrackedFandoms->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::OnNewSelectionInRecentList);
    connect(ui->lvRecommenders->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::OnNewSelectionInRecommenderList);
}


#define ADD_STRING_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const Section* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (Section* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toString(); \
    } \
    ); \

#define ADD_DATE_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const Section* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (Section* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toDateTime(); \
    } \
    ); \

#define ADD_INTEGER_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const Section* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (Section* data, QVariant value) \
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
    ADD_STRING_GETSET(holder, 1, 0, author);
    ADD_STRING_GETSET(holder, 2, 0, title);
    ADD_STRING_GETSET(holder, 3, 0, summary);
    ADD_STRING_GETSET(holder, 4, 0, genre);
    ADD_STRING_GETSET(holder, 5, 0, characters);
    ADD_STRING_GETSET(holder, 6, 0, rated);
    ADD_DATE_GETSET(holder, 7, 0, published);
    ADD_DATE_GETSET(holder, 8, 0, updated);
    ADD_STRING_GETSET(holder, 9, 0, url);
    ADD_STRING_GETSET(holder, 10, 0, tags);
    ADD_INTEGER_GETSET(holder, 11, 0, wordCount);
    ADD_INTEGER_GETSET(holder, 12, 0, favourites);
    ADD_INTEGER_GETSET(holder, 13, 0, reviews);
    ADD_INTEGER_GETSET(holder, 14, 0, chapters);
    ADD_INTEGER_GETSET(holder, 15, 0, complete);
    ADD_INTEGER_GETSET(holder, 16, 0, atChapter);
    ADD_INTEGER_GETSET(holder, 17, 0, ID);
    ADD_INTEGER_GETSET(holder, 18, 0, recommendations);

    //    holder->AddGetter(QPair<int,int>(8,0),
    //    [] (const Section* )
    //    {
    //        return QVariant(QString(" "));
    //    });

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
    holder = new TableDataListHolder<Section>();
    //rollbackBuiltins = GenerationSettings::builtins;
    typetableModel = new FicModel();

    SetupTableAccess();


    holder->SetColumns(QStringList() << "fandom" << "author" << "title" << "summary" << "genre" << "characters" << "rated" << "published"
                       << "updated" << "url" << "tags" << "wordCount" << "favourites" << "reviews" << "chapters" << "complete" << "atChapter" << "ID" << "recommendations");

    typetableInterface = QSharedPointer<TableDataInterface>(dynamic_cast<TableDataInterface*>(holder));

    typetableModel->SetInterface(typetableInterface);

    //sortModel = new QSortFilterProxyModel();
    //sortModel->setSourceModel(typetableModel);
    //sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    //types_table->setModel();
    holder->SetData(fanfics);
    //ui->tvFanfics->setModel(typetableModel);

    //ui->wdgFicviewPlaceholder->resize(400,400);
    qwFics = new QQuickWidget();
    QHBoxLayout* lay = new QHBoxLayout;
    lay->addWidget(qwFics);
    ui->wdgFicviewPlaceholder->setLayout(lay);
    qwFics->setResizeMode(QQuickWidget::SizeRootObjectToView);
    qwFics->rootContext()->setContextProperty("ficModel", typetableModel);
    //tagModel = new QStringListModel(tagList);

    qwFics->rootContext()->setContextProperty("tagModel", tagList);
    //qwFics->rootContext()->setContextProperty("ficModel", new QStringListModel(QStringList() << "Naruto"));
    QUrl source("qrc:/qml/ficview.qml");
    qwFics->setSource(source);

    QObject *childObject = qwFics->rootObject()->findChild<QObject*>("lvFics");
    connect(childObject, SIGNAL(chapterChanged(QVariant, QVariant, QVariant)), this, SLOT(OnChapterUpdated(QVariant, QVariant, QVariant)));
    connect(childObject, SIGNAL(tagClicked(QVariant, QVariant, QVariant)), this, SLOT(OnTagClicked(QVariant, QVariant, QVariant)));
    connect(childObject, SIGNAL(tagAdded(QVariant, QVariant)), this, SLOT(OnTagAdd(QVariant,QVariant)));
    connect(childObject, SIGNAL(tagDeleted(QVariant, QVariant)), this, SLOT(OnTagRemove(QVariant,QVariant)));
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
    settings.setValue("Settings/active", ui->chkActive->isChecked());
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
    settings.setValue("Settings/lengthCutoff", ui->cbWordCutoff->currentText());
    //    QString temp;
    //    for(int size : ui->spMain->sizes())
    //        temp.push_back(QString::number(size) + " ");
    //    settings.setValue("Settings/spMain", temp.trimmed());
    settings.sync();
    delete ui;
}

void MainWindow::Init()
{
    names.clear();
    UpdateFandomList(fandomManager, [](Fandom f){return f.url;});
    UpdateFandomList(crossoverManager, [](Fandom f){return f.crossoverUrl;});
    InsertFandomData(names);
    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));

    //ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));
}

void MainWindow::IntiConnections()
{
    connect(ui->chkCustomFilter, &QCheckBox::clicked, this, &MainWindow::OnCustomFilterClicked);
    connect(ui->chkActivateReloadSectionData, &QCheckBox::clicked, this, &MainWindow::OnSectionReloadActivated);

}

void MainWindow::RequestAndProcessPage(QString page)
{
    nextUrl = page;
    do
    {
        An<PageManager> pager;
        bool cacheMode = ui->chkCacheMode->isChecked();
        WebPage result = pager->GetPage(nextUrl, cacheMode);
        if(!result.isValid)
        {
            ui->edtResults->append(" Не удалось получить страницу");
            return;
        }
        ProcessPage(QString(result.content));
        result.fandom =  currentFandom;
        result.crossover = isCrossover;
        result.type = EPageType::sorted_ficlist;
        if(result.source == EPageSource::network)
        {
            pager->SavePageToDB(result);
            QThread::msleep(1000);
        }
        pbMain->setValue((pbMain->value()+10)%pbMain->maximum());
    }while(!nextUrl.isEmpty());
    pbMain->setValue(0);
    pbMain->hide();
    //manager.get(QNetworkRequest(QUrl(page)));
}

WebPage MainWindow::RequestPage(QString pageUrl)
{
    WebPage result;
    QString toInsert = "<a href=\"" + pageUrl + "\"> %1 </a>";
    toInsert= toInsert.arg(pageUrl);
    ui->edtResults->append("<span>Processing url: </span>");
    ui->edtResults->insertHtml(toInsert);
    An<PageManager> pager;
    pager->SetDatabase(QSqlDatabase::database(dbName));
    bool cacheMode = ui->chkCacheMode->isChecked();
    pbMain->setTextVisible(false);
    pbMain->show();

    result = pager->GetPage(pageUrl, cacheMode);
    return result;
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

        section.fandom = ui->rbNormal->isChecked() ? currentFandom: currentFandom + " CROSSOVER";
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

        if(section.updated.toMSecsSinceEpoch() < lastUpdated.toMSecsSinceEpoch() && section.updated.date().year() > 2000 && !ignoreUpdateDate)
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
        nextUrl = "";
        return;
    }


    for(auto section : sections)
    {
        section.origin = currentFilterUrl;
        if(database::LoadIntoDB(section))
            processedFics++;
        ui->edtResults->insertHtml("<span> Written:" + section.title + " by " + section.author + "\n <br></span>");
    }
    if(abort)
    {
        ui->edtResults->insertHtml("<span> Already have updates past that point, aborting<br></span>");
        nextUrl = "";
        return;
    }
    if(sections.size() > 0)
        GetNext(sections.last(), currentPosition, str);
    currentPosition = 999;
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

inline Section LoadFanfic(QSqlQuery& q)
{
    Section result;
    result.ID = q.value("ID").toInt();
    result.fandom = q.value("FANDOM").toString();
    result.author = q.value("AUTHOR").toString();
    result.title = q.value("TITLE").toString();
    result.summary = q.value("SUMMARY").toString();
    result.genre= q.value("GENRES").toString();
    result.characters = q.value("CHARACTERS").toString().replace("not found", "");
    result.rated = q.value("RATED").toString();
    result.published = q.value("PUBLISHED").toDateTime();
    result.updated= q.value("UPDATED").toDateTime();
    result.url = q.value("URL").toString();
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

        if(counter%100 == 0)
            qDebug() << "tick " << counter/100;
    }
    qDebug() << "loaded fics:" << counter;

}

QSqlQuery MainWindow::BuildQuery()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString queryString = "select ID, ";
    //if(ui->cbCustomFilters->currentText() == "New From Popular")
    queryString+= " (SELECT  Sum(favourites) as summation FROM fanfics where author = f.author) as sumfaves, ";
    queryString+= " (SELECT  count(fic_id) FROM recommendations where f.id = fic_id) as sumrecs, ";
    queryString+=" f.* from fanfics f where 1 = 1 " ;
    if(ui->cbMinWordCount->currentText().toInt() > 0)
        queryString += " and wordcount > :minwordcount ";
    if(ui->cbMaxWordCount->currentText().toInt() > 0)
        queryString += " and wordcount < :maxwordcount ";

    if(ui->chkFaveLimitActivated->isChecked() && ui->sbMinimumFavourites->value() > 0)
        queryString += " and favourites > :favourites ";

    if(!ui->leContainsGenre->text().trimmed().isEmpty())
        for(auto genre : ui->leContainsGenre->text().trimmed().split(" "))
            queryString += QString(" AND genres like '%%1%' ").arg(genre);

    if(!ui->leNotContainsGenre->text().trimmed().isEmpty())
        for(auto genre : ui->leNotContainsGenre->text().trimmed().split(" "))
            queryString += QString(" AND genres not like '%%1%' ").arg(genre);

    for(QString word: ui->leContainsWords->text().trimmed().split(" "))
    {
        if(word.trimmed().isEmpty())
            continue;
        queryString += QString(" AND ((summary like '%%1%' and summary not like '%not %1%') or (title like '%%1%' and title not like '%not %1%') ) ").arg(word);
    }

    for(QString word: ui->leNotContainsWords->text().trimmed().split(" "))
    {
        if(word.trimmed().isEmpty())
            continue;
        queryString += QString(" AND summary not like '%%1%' and summary not like '%not %1%'").arg(word);
    }

    if(currentSearchButton == MainWindow::lfbp_recs)
    {
        QString qsl = " and id in (select fic_id from recommendations %1)";

        if(!ui->leAuthorUrl->text().trimmed().isEmpty())
            qsl=qsl.arg(QString(" where recommender_id = %1 ").arg(QString::number(currentRecommenderId)));
        else
            qsl=qsl.arg(QString(""));
        queryString+=qsl;
    }

    QString tags, not_tags;

    QString diffField;
    if(ui->cbSortMode->currentText() == "Wordcount")
        diffField = " WORDCOUNT DESC";
    else if(ui->cbSortMode->currentText() == "Favourites")
        diffField = " FAVOURITES DESC";
    else if(ui->cbSortMode->currentText() == "Update Date")
        diffField = " updated DESC";
    else if(ui->cbSortMode->currentText() == "Rec Count")
        diffField = " sumrecs desc";
    else if(ui->cbSortMode->currentText() == "Fav Rate")
        diffField = " favourites/(julianday(Updated) - julianday(Published)) desc";
//        diffField = " wordcount/favourites asc";
//    else




    bool tagsMatter = true;
    if(ui->chkCustomFilter->isChecked() && ui->cbCustomFilters->currentText() == "Longest Running")
    {
        queryString =  "select ID, julianday(f.updated) - julianday(f.published) as datediff, f.* from fanfics f where 1 = 1 and datediff > 0  ";
        diffField = "datediff desc";
        tagsMatter = false;
    }
    if(ui->chkCustomFilter->isChecked() && ui->cbCustomFilters->currentText() == "Behemoth Chapters")
    {
        queryString = "select ID, f.wordcount / f.chapters as datediff, f.* from fanfics f where 1 = 1 ";
        diffField = "datediff desc";
        tagsMatter = false;
    }
    if(ui->chkCustomFilter->isChecked() && ui->cbCustomFilters->currentText() == "Splinched")
    {
        queryString = "select ID, f.wordcount / f.chapters as datediff, f.* from fanfics f where 1 = 1 ";
        diffField = "datediff asc";
        tagsMatter = false;
    }
    if(ui->chkCustomFilter->isChecked() && ui->cbCustomFilters->currentText() == "Catch'Em'All")
    {
        queryString = "select ID, f.* from fanfics f where 1 = 1 and length(characters) > 40 ";
        diffField = "length(characters) desc";
        tagsMatter = false;
    }
    if(ui->chkCustomFilter->isChecked() && ui->cbCustomFilters->currentText() == "Dumped")
    {
        queryString+=QString(" and abs(cast("
                             "("
                             " strftime('%s',f.updated)-strftime('%s',f.published) "
                             " ) AS real "
                             " )/60/60/24) < 60 ");

        //tagsMatter = false;
    }


    if(ui->chkCustomFilter->isChecked() && ui->cbCustomFilters->currentText() == "One Liners")
    {
        queryString = "select ID, f.* from fanfics f where 1 = 1 and summary not like '%sequel%'"
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

    if(ui->rbNormal->isChecked())
        queryString+=QString(" and  fandom like '%%1%'").arg(ui->cbNormals->currentText());
    else if(ui->rbCrossovers->isChecked())
        queryString+=QString(" and  fandom like '%%1%' and fandom like '%CROSSOVER%'").arg(ui->cbNormals->currentText());
    database::PushFandom(ui->cbNormals->currentText().trimmed());
    //ui->cbNormals->blockSignals(true);
    recentFandomsModel->setStringList(database::FetchRecentFandoms());
    ui->lvTrackedFandoms->setModel(recentFandomsModel);
    //ui->cbNormals->blockSignals(false);
    for(auto tag : ui->wdgTagsPlaceholder->GetSelectedTags())
        tags.push_back(WrapTag(tag) + "|");

    tags.chop(1);
    not_tags.chop(1);

    //QString queryString;
    if(currentSearchButton == MainWindow::lfbp_search)
    {
        if(tagsMatter)
        {
            if(!tags.isEmpty())
                queryString +=" and tags regexp :tags ";
            else
                queryString +=" and tags = ' none ' ";
        }
    }
    else
    {
        if(ui->chkShowRecsRegardlessOfTags->isChecked())
            queryString += QString("");
        else
            queryString += QString(" and tags = ' none ' ");
    }

    if(ui->cbSortMode->currentText() == "Fav Rate")
    {
        queryString+= " and published <> updated and published > date('now', '-1 years') and updated > date('now', '-60 days') ";
    }

    if(ui->cbSortMode->currentText() == "New From Popular")
    {
        queryString+="and published > date('now', '-1 years') and updated > date('now', '-60 days')";
        diffField = " sumfaves desc";

        //tagsMatter = false;
    }


    queryString+="COLLATE NOCASE ORDER BY " + diffField;
    queryString+=CreateLimitQueryPart();

    std::function<QSqlQuery(QString)> bindQuery =
            [&](QString qs){
        QSqlQuery q(db);
        q.prepare(qs);

        if(ui->cbMinWordCount->currentText().toInt() > 0)
            q.bindValue(":minwordcount", ui->cbMinWordCount->currentText().toInt());
        if(ui->cbMaxWordCount->currentText().toInt() > 0)
            q.bindValue(":maxwordcount", ui->cbMaxWordCount->currentText().toInt());
        if(ui->sbMinimumFavourites->value() > 0)
            q.bindValue(":favourites", ui->sbMinimumFavourites->value());

        q.bindValue(":tags", tags);
        q.bindValue(":not_tags", not_tags);
        return q;
    };

    int maxFicCountValue = ui->chkFicLimitActivated->isChecked() ? ui->sbMaxFicCount->value()  : 0;
    if(ui->chkRandomizeSelection->isChecked() && maxFicCountValue > 0 && maxFicCountValue < 51)
    {
        PopulateIdList(bindQuery, queryString);
    }
    auto q = bindQuery(AddIdList(queryString, maxFicCountValue));
    return q;
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
        q.prepare(QString("update fanfics set tags = tags || :tag where ID = :id and tags not regexp :wrappedTag"));
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

void MainWindow::InsertFandomData(QMap<QPair<QString,QString>, Fandom> names)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QHash<QPair<QString, QString>, Fandom> knownValues;
    for(auto value : sections)
    {

        QString qs = QString("Select section, fandom, normal_url, crossover_url from fandoms where section = '%1'").
                arg(value.name.replace("'","''"));
        QSqlQuery q(qs, db);


        while(q.next())
        {
            knownValues[{q.value("section").toString(),
                    q.value("fandom").toString()}] =
                    Fandom{q.value("fandom").toString(),
                    q.value("section").toString(),
                    q.value("normal_url").toString(),
                    q.value("crossover_url").toString(),};
        }
        qDebug() << q.lastError();
    }
    auto make_key = [](Fandom f){return QPair<QString, QString>(f.section, f.name);} ;
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


void MainWindow::UpdateFandomList(QNetworkAccessManager& manager,
                                  std::function<QString(Fandom)> linkGetter)
{
    for(auto value : sections)
    {
        currentProcessedSection = value.name;
        manager.get(QNetworkRequest(QUrl(linkGetter(value))));
        managerEventLoop.exec();
    }
}

QStringList MainWindow::GetFandomListFromDB()
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select fandom from fandoms where section ='%1' and normal_url is not null").arg(ui->cbSectionTypes->currentText());
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
    QSqlDatabase db = QSqlDatabase::database(dbName);
    //QString qs = QString("Select crossover_url from fandoms where section = '%1' and fandom = '%2'").arg(ui->cbSectionTypes->currentText()).arg(ui->cbNormals->currentText());
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
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select normal_url from fandoms where fandom = '%1' ").arg(fandom);
    if(false)
        qs+=" and (tracked = 1 or tracked_crossovers = 1)";
    //qDebug() << qs;
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
    QString qs = QString("update fanfics set tags = tags || ' ' || :tag where ID = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag", tag);
    q.bindValue(":id",id);
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();


    if(!tagList.contains(tag))
    {
        q.prepare("INSERT INTO TAGS(TAG) VALUES(:tag)");
        q.bindValue(":tag", tag);
        q.exec();
        if(q.lastError().isValid())
            qDebug() << q.lastError();
    }
}

void MainWindow::UnsetTag(int id, QString tag)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);

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

QDateTime MainWindow::GetMaxUpdateDateForSection(QStringList sections)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QString qs = QString("Select max(updated) as updated from fanfics where 1 = 1 %1");
    QString append;
    if(sections.size() == 1)
    {
        append+= QString(" and fandom = '%1' ").arg(sections.at(0));
    }
    else
    {
        for(auto section : sections)
        {
            append+= QString(" and fandom like '%%1%' ").arg(section);
        }
    }
    qs=qs.arg(append);
    QSqlQuery q(qs, db);
    q.next();
    qDebug() << q.lastQuery();
    //QDateTime result = q.value("updated").toDateTime();
    QString resultStr = q.value("updated").toString();


    QDateTime result;
    if(!ui->chkIgnoreUpdateDate->isChecked())
        result = QDateTime::fromString(resultStr, "yyyy-MM-ddThh:mm:ss.000");
    else
        result = QDateTime();
    return result;
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
        lastUpdated = GetMaxUpdateDateForSection(QStringList() << selectedFandom << "CROSSOVER");
        isCrossover = true;
        currentFandom = selectedFandom;
    }
    else
    {

        isCrossover = false;
        urls = GetNormalUrl(selectedFandom,ignoreTrackingState);
        lastUpdated = GetMaxUpdateDateForSection(QStringList() << selectedFandom);
        currentFandom = selectedFandom;

    }
    return urls;
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
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QString qs = QString("delete from fanfics where fandom like '%%1%'");
        qs=qs.arg(fandom);
        QSqlQuery q(qs, db);
        q.exec();
    }
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
    //ui->cbCrossovers->setCurrentText(settings.value("Settings/crossovers", "").toString());
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


    ui->chkActive->setChecked(settings.value("Settings/active", false).toBool());
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
        names.insert({currentProcessedSection,name}, Fandom{name, currentProcessedSection, link, ""});
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
    //int indexOfSlash = currentProcessedSection.indexOf("/");
    QString pattern = sections[currentProcessedSection].section;//.mid(indexOfSlash + 1, currentProcessedSection.length() - (indexOfSlash +1)) + "\\sCrossovers";
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
            names.insert({currentProcessedSection,name},Fandom{name, currentProcessedSection,  "",link});
        else
            names[{currentProcessedSection,name}].crossoverUrl = link;
    }
    managerEventLoop.quit();
}

void MainWindow::OnChapterUpdated(QVariant chapter, QVariant author, QVariant title)
{
    QSqlDatabase db = QSqlDatabase::database(dbName);
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
        RequestAndProcessPage(url);
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
        //p.setColor(QPalette::Button, QColor("DarkGreen"));
        //p.setColor(QPalette::Active, QPalette::Button, Qt::white);
        //p.setColor(QPalette::Inactive, QPalette::Button, Qt::white);
        //ui->cbCustomFilters->setStyleSheet("QComboBox{ border: 1px solid green;}");
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
    ui->cbNormals->setModel(new QStringListModel(GetFandomListFromDB()));
    //ui->cbCrossovers->setModel(new QStringListModel(GetCrossoverListFromDB()));
}

void MainWindow::on_pbLoadDatabase_clicked()
{
    currentSearchButton = MainWindow::lfbp_search;
    LoadData();

    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
    //typetableModel->OnReloadDataFromInterface();
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
    if(currentSearchButton == MainWindow::lfbp_search)
        on_pbLoadDatabase_clicked();
    else
        on_pbLoadPage_clicked();
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
        ui->leAuthorUrl->setText(recommenders[recommender].url);
}

//void MainWindow::OnAcceptedExpandedWidget(QString)
//{

//}

void MainWindow::CallExpandedWidget()
{
    if(!expanderWidget)
    {
        expanderWidget = new QDialog();
        //expanderWidget->setWindowModality(Qt::WindowModal);
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
    for(QString fandom : database::FetchTrackedFandoms())
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
            RequestAndProcessPage(url);
        }
    }
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
            RequestAndProcessPage(url);
        }
    }
    QMessageBox::information(nullptr, "Info", QString("finished processing %1 fics" ).arg(processedFics));
    //ui->sb
}

void MainWindow::on_pbLoadPage_clicked()
{
    currentSearchButton = MainWindow::lfbp_recs;
    auto startPageRequest = std::chrono::high_resolution_clock::now();
    auto page = RequestPage(ui->leAuthorUrl->text());
    auto elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
    qDebug() << "Fetched page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    FavouriteStoryParser parser;
    auto startPageProcess = std::chrono::high_resolution_clock::now();
    parser.ProcessPage(page.url, QString(page.content));
    elapsed = std::chrono::high_resolution_clock::now() - startPageProcess;
    qDebug() << "Processed page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->clear();
    ui->edtResults->insertHtml(parser.diagnostics.join(""));
    auto startRecLoad = std::chrono::high_resolution_clock::now();

    currentRecommenderId = database::GetRecommenderId(page.url);
    //auto query = BuildQuery();
    //LoadRecommendations(page.url);

    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));
    //recommendersModel->setStringList(recommenders.keys());

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
        database::RemoveRecommender(recommenders[currentSelection]);
        recommenders = database::FetchRecommenders();
        recommendersModel->setStringList(SortedList(recommenders.keys()));
    }
}

void MainWindow::on_pbOpenRecommendations_clicked()
{

    currentSearchButton = MainWindow::lfbp_recs;

    auto startRecLoad = std::chrono::high_resolution_clock::now();

    currentRecommenderId = database::GetRecommenderId(ui->leAuthorUrl->text());
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
    currentSearchButton = MainWindow::lfbp_recs;
    auto startPageRequest = std::chrono::high_resolution_clock::now();
    for(auto recommender: recommenders.values())
    {
        auto page = RequestPage(recommender.url);
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
        qDebug() << "Fetched page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        FavouriteStoryParser parser;
        auto startPageProcess = std::chrono::high_resolution_clock::now();
        parser.ProcessPage(page.url, QString(page.content));
        elapsed = std::chrono::high_resolution_clock::now() - startPageProcess;
        qDebug() << "Processed page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->clear();
        ui->edtResults->insertHtml(parser.diagnostics.join(""));

    }
    ui->leAuthorUrl->setText("");
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    currentRecommenderId = database::GetRecommenderId(ui->leAuthorUrl->text());
    //auto query = BuildQuery();
    //LoadRecommendations(page.url);

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
    currentSearchButton = MainWindow::lfbp_recs;
    ui->leAuthorUrl->setText("");
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    currentRecommenderId = database::GetRecommenderId(ui->leAuthorUrl->text());
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
    currentSearchButton = MainWindow::lfbp_recs;
    auto startPageRequest = std::chrono::high_resolution_clock::now();
    for(auto recommender: recommenders.values())
    {
        auto page = RequestPage(recommender.url);
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
        qDebug() << "Fetched page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        FavouriteStoryParser parser;
        auto startPageProcess = std::chrono::high_resolution_clock::now();
        auto sections = parser.ProcessPage(page.url, QString(page.content));
        for(auto section: sections)
        {
            QString toInsert = "<a href=\"" + section.authorUrl + "\"> %1 </a>";
            toInsert= toInsert.arg(pageUrl);
            ui->edtResults->append("<span>Processing url: </span>");
            ui->edtResults->insertHtml(toInsert);

            auto page = RequestPage(section.authorUrl);
            auto id = database::GetRecommenderId(section.authorUrl);
            if(id == -1)
            {
                FavouriteStoryParser parser;
                parser.ProcessPage(page.url, QString(page.content),1);
                auto id = database::GetRecommenderId(section.authorUrl);
                database::FilterRecommenderByRecField(id, 5);
            }
        }


        elapsed = std::chrono::high_resolution_clock::now() - startPageProcess;
        qDebug() << "Processed page in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->clear();
        ui->edtResults->insertHtml(parser.diagnostics.join(""));

    }
    ui->leAuthorUrl->setText("");
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    currentRecommenderId = database::GetRecommenderId(ui->leAuthorUrl->text());
    //auto query = BuildQuery();
    //LoadRecommendations(page.url);

    recommenders = database::FetchRecommenders();
    recommendersModel->setStringList(SortedList(recommenders.keys()));
    currentRecWave = 1;
    LoadData();
    currentRecWave = 0;
    auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(fanfics);
}
