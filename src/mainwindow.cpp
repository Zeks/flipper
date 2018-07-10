/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GlobalHeaders/SingletonHolder.h"
#include "GlobalHeaders/simplesettings.h"
#include "GlobalHeaders/SignalBlockerWrapper.h"
#include "Interfaces/recommendation_lists.h"
#include "Interfaces/authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/pagetask_interface.h"
#include "actionprogress.h"
#include "ui_actionprogress.h"
#include "pagetask.h"
#include "timeutils.h"
#include "regex_utils.h"
#include "Interfaces/tags.h"
#include "EGenres.h"
#include <QMessageBox>
#include <QReadWriteLock>
#include <QReadLocker>
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
#include <QMovie>
#include <QVector>
#include <QFileDialog>
#include <chrono>
#include <algorithm>
#include <math.h>

#include "genericeventfilter.h"


#include "include/parsers/ffn/favparser.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/parsers/ffn/fandomindexparser.h"
#include "include/url_utils.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/tasks/author_task_processor.h"
#include "include/tasks/author_stats_processor.h"
#include "include/tasks/slash_task_processor.h"
#include "include/tasks/humor_task_processor.h"
#include "include/tasks/recommendations_reload_precessor.h"
#include "include/tasks/fandom_list_reload_processor.h"
#include "include/page_utils.h"
#include "include/environment.h"
#include "include/generic_utils.h"
#include "include/statistics_utils.h"


#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/recommendation_lists.h"
#include "Interfaces/tags.h"
#include "Interfaces/genres.h"

struct TaskProgressGuard
{
    TaskProgressGuard(MainWindow* w){
        w->SetWorkingStatus();
        this->w = w;
    }
    ~TaskProgressGuard(){
        if(!hasFailures)
            w->SetFinishedStatus();
        else
            w->SetFailureStatus();
    }
    bool hasFailures = false;
    MainWindow* w = nullptr;

};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    refreshSpin(":/icons/icons/refresh_spin_lg.gif")
{
    ui->setupUi(this);
    ui->cbNormals->lineEdit()->setClearButtonEnabled(true);
    ui->leContainsWords->setClearButtonEnabled(true);
    ui->leContainsGenre->setClearButtonEnabled(true);
    ui->leNotContainsWords->setClearButtonEnabled(true);
    ui->leNotContainsGenre->setClearButtonEnabled(true);

}

bool MainWindow::Init()
{
    QSettings settings("settings.ini", QSettings::IniFormat);

    if(!env.Init())
        return false;

    this->setWindowTitle("Flipper");
    this->setAttribute(Qt::WA_QuitOnClose);


    ui->chkShowDirectRecs->setVisible(false);
    ui->pbFirstWave->setVisible(false);

    if(settings.value("Settings/hideCache", true).toBool())
        ui->chkCacheMode->setVisible(false);

    ui->dteFavRateCut->setDate(QDate::currentDate().addDays(-366));
    ui->pbLoadDatabase->setStyleSheet("QPushButton {background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   stop:0 rgba(179, 229, 160, 128), stop:1 rgba(98, 211, 162, 128))}"
                                      "QPushButton:hover {background-color: #9cf27b; border: 1px solid black;border-radius: 5px;}"
                                      "QPushButton {background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,   stop:0 rgba(179, 229, 160, 128), stop:1 rgba(98, 211, 162, 128))}");



    ui->wdgTagsPlaceholder->fandomsInterface = env.interfaces.fandoms;
    ui->wdgTagsPlaceholder->tagsInterface = env.interfaces.tags;
//    tagWidgetDynamic->fandomsInterface = env.interfaces.fandoms;
//    tagWidgetDynamic->tagsInterface = env.interfaces.tags;

    recentFandomsModel = new QStringListModel;
    ignoredFandomsModel = new QStringListModel;
    ignoredFandomsSlashFilterModel= new QStringListModel;
    recommendersModel= new QStringListModel;



    ui->edtResults->setContextMenuPolicy(Qt::CustomContextMenu);

    auto fandomList = env.interfaces.fandoms->GetFandomList(true);
    ui->cbNormals->setModel(new QStringListModel(fandomList));
    ui->cbIgnoreFandomSelector->setModel(new QStringListModel(fandomList));
    ui->cbIgnoreFandomSlashFilter->setModel(new QStringListModel(fandomList));
    ui->deCutoffLimit->setEnabled(false);

    actionProgress = new ActionProgress;
    pbMain = actionProgress->ui->pbMain;
    pbMain->setMinimumWidth(200);
    pbMain->setMaximumWidth(200);
    pbMain->setValue(0);
    pbMain->setTextVisible(false);

    lblCurrentOperation = new QLabel;
    ui->statusBar->addPermanentWidget(lblCurrentOperation,1);
    ui->statusBar->addPermanentWidget(actionProgress,0);

    ui->edtResults->setOpenLinks(false);
    auto showTagWidget = settings.value("Settings/showNewTagsWidget", false).toBool();
    if(!showTagWidget)
        ui->tagWidget->removeTab(1);

    recentFandomsModel->setStringList(env.interfaces.fandoms->GetRecentFandoms());
    ignoredFandomsModel->setStringList(env.interfaces.fandoms->GetIgnoredFandoms());
    ignoredFandomsSlashFilterModel->setStringList(env.interfaces.fandoms->GetIgnoredFandomsSlashFilter());
    ui->lvTrackedFandoms->setModel(recentFandomsModel);
    ui->lvIgnoredFandoms->setModel(ignoredFandomsModel);
    ui->lvExcludedFandomsSlashFilter->setModel(ignoredFandomsSlashFilterModel);


    ProcessTagsIntoGui();
    FillRecTagCombobox();



    SetupFanficTable();
    FillRecommenderListView();
    //CreatePageThreadWorker();
    callProgress = [&](int counter) {
        pbMain->setTextVisible(true);
        pbMain->setFormat("%v");
        pbMain->setValue(counter);
        pbMain->show();
    };
    callProgressText = [&](QString value) {
        ui->edtResults->insertHtml(value);
        ui->edtResults->ensureCursorVisible();
    };
    cleanupEditor = [&](void) {
        ui->edtResults->clear();
    };

    fandomMenu.addAction("Remove fandom from list", this, SLOT(OnRemoveFandomFromRecentList()));
    ignoreFandomMenu.addAction("Remove fandom from list", this, SLOT(OnRemoveFandomFromIgnoredList()));
    ignoreFandomSlashFilterMenu.addAction("Remove fandom from list", this, SLOT(OnRemoveFandomFromSlashFilterIgnoredList()));
    ui->lvTrackedFandoms->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->lvIgnoredFandoms->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->lvExcludedFandomsSlashFilter->setContextMenuPolicy(Qt::CustomContextMenu);
    //ui->edtResults->setOpenExternalLinks(true);
    ui->cbWordCutoff->setVisible(false);
    ui->edtRecsContents->setReadOnly(false);
    ui->wdgDetailedRecControl->hide();
    bool thinClient = settings.value("Settings/thinClient").toBool();
    if(thinClient)
        SetClientMode();
    ResetFilterUItoDefaults();
    ReadSettings();
    //    ui->spRecsFan->setStretchFactor(0, 0);
    //    ui->spRecsFan->setStretchFactor(1, 1);
    ui->spFanIgnFan->setCollapsible(0,0);
    ui->spFanIgnFan->setCollapsible(1,0);
    ui->spFanIgnFan->setSizes({1000,0});
    ui->spRecsFan->setCollapsible(0,0);
    ui->spRecsFan->setCollapsible(1,0);
    ui->wdgDetailedRecControl->hide();
    ui->spRecsFan->setSizes({0,1000});
    ui->wdgSlashFandomExceptions->hide();
    ui->chkEnableSlashExceptions->hide();
    return true;
}

void MainWindow::InitConnections()
{
    connect(ui->pbCopyAllUrls, SIGNAL(clicked(bool)), this, SLOT(OnCopyAllUrls()));
    connect(ui->pbOpenID, SIGNAL(clicked(bool)), this, SLOT(OnOpenAuthorListByID()));
    connect(ui->pbGetFavouriteLinks, &QPushButton::clicked, this, &MainWindow::OnGetAuthorFavouritesLinks);
    connect(ui->pbPauseTask, &QPushButton::clicked, [&](){
        cancelCurrentTaskPressed = true;
    });
    connect(ui->pbContinueTask, &QPushButton::clicked, [&](){
        auto task = env.interfaces.pageTask->GetCurrentTask();
        if(!task)
        {
            QMessageBox::warning(nullptr, "Warning!", "No task to continue!");
            return;
        }
        InitUIFromTask(task);
        UseAuthorTask(task);

    });


    connect(ui->wdgTagsPlaceholder, &TagWidget::tagToggled, this, &MainWindow::OnTagToggled);
    connect(ui->pbCopyFavUrls, &QPushButton::clicked, this, &MainWindow::OnCopyFavUrls);
    connect(ui->wdgTagsPlaceholder, &TagWidget::refilter, [&](){
        qwFics->rootContext()->setContextProperty("ficModel", nullptr);

        if(ui->gbTagFilters->isChecked() && ui->wdgTagsPlaceholder->GetSelectedTags().size() > 0)
        {
            env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_fics);
            if(env.filter.isValid)
                LoadData();
        }
        if(ui->gbTagFilters->isChecked()  && ui->wdgTagsPlaceholder->GetSelectedTags().size() == 0)
            on_pbLoadDatabase_clicked();
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(env.fanfics);
        typetableModel->OnReloadDataFromInterface();
        qwFics->rootContext()->setContextProperty("ficModel", typetableModel);
    });

    connect(ui->wdgTagsPlaceholder, &TagWidget::tagDeleted, [&](QString tag){
        ui->wdgTagsPlaceholder->OnRemoveTagFromEdit(tag);

        if(tagList.contains(tag))
        {
            env.interfaces.tags->DeleteTag(tag);
            tagList.removeAll(tag);
            qwFics->rootContext()->setContextProperty("tagModel", tagList);
        }
    });
    connect(ui->wdgTagsPlaceholder, &TagWidget::tagAdded, [&](QString tag){
        if(!tagList.contains(tag))
        {
            env.interfaces.tags->CreateTag(tag);
            tagList.append(tag);
            qwFics->rootContext()->setContextProperty("tagModel", tagList);
        }

    });
    connect(ui->wdgTagsPlaceholder, &TagWidget::dbIDRequest, this, &MainWindow::OnFillDBIdsForTags);
    connect(ui->wdgTagsPlaceholder, &TagWidget::tagReloadRequested, this, &MainWindow::OnTagReloadRequested);
    connect(&taskTimer, &QTimer::timeout, this, &MainWindow::OnCheckUnfinishedTasks);
    connect(ui->lvTrackedFandoms->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::OnNewSelectionInRecentList);
    //! todo currently null
    connect(ui->lvRecommenders->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::OnNewSelectionInRecommenderList);

    connect(ui->lvTrackedFandoms, &QListView::customContextMenuRequested, this, &MainWindow::OnFandomsContextMenu);
    connect(ui->lvIgnoredFandoms, &QListView::customContextMenuRequested, this, &MainWindow::OnIgnoredFandomsContextMenu);
    connect(ui->lvExcludedFandomsSlashFilter, &QListView::customContextMenuRequested, this, &MainWindow::OnIgnoredFandomsSlashFilterContextMenu);
    connect(ui->edtResults, &QTextBrowser::anchorClicked, this, &MainWindow::OnOpenLogUrl);
    connect(ui->edtRecsContents, &QTextBrowser::anchorClicked, this, &MainWindow::OnOpenLogUrl);
    connect(ui->pbWipeCache, &QPushButton::clicked, this, &MainWindow::OnWipeCache);
    connect(ui->pbAssignGenres, &QPushButton::clicked, this, &MainWindow::OnPerformGenreAssignment);

    connect(&env, &CoreEnvironment::resetEditorText, this, &MainWindow::OnResetTextEditor);
    connect(&env, &CoreEnvironment::requestProgressbar, this, &MainWindow::OnProgressBarRequested);
    connect(&env, &CoreEnvironment::updateCounter, this, &MainWindow::OnUpdatedProgressValue);
    connect(&env, &CoreEnvironment::updateInfo, this, &MainWindow::OnNewProgressString);

}

void MainWindow::InitUIFromTask(PageTaskPtr task)
{
    if(!task)
        return;
    AddToProgressLog("Authors: " + QString::number(task->size));
    ReinitProgressbar(task->size);
    DisableAllLoadButtons();
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

#define ADD_STRING_INTEGER_GETSET(HOLDER,ROW,ROLE,PARAM)  \
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
    data->PARAM = QString::number(value.toInt()); \
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
    ADD_STRING_GETSET(holder, 1, 0, author->name);
    ADD_STRING_GETSET(holder, 2, 0, title);
    ADD_STRING_GETSET(holder, 3, 0, summary);
    ADD_STRING_GETSET(holder, 4, 0, genreString);
    ADD_STRING_GETSET(holder, 5, 0, charactersFull);
    ADD_STRING_GETSET(holder, 6, 0, rated);
    ADD_DATE_GETSET(holder, 7, 0, published);
    ADD_DATE_GETSET(holder, 8, 0, updated);
    ADD_STRING_GETSET(holder, 9, 0, urlFFN);
    ADD_STRING_GETSET(holder, 10, 0, tags);
    ADD_STRING_INTEGER_GETSET(holder, 11, 0, wordCount);
    ADD_STRING_INTEGER_GETSET(holder, 12, 0, favourites);
    ADD_STRING_INTEGER_GETSET(holder, 13, 0, reviews);
    ADD_STRING_INTEGER_GETSET(holder, 14, 0, chapters);
    ADD_INTEGER_GETSET(holder, 15, 0, complete);
    ADD_INTEGER_GETSET(holder, 16, 0, atChapter);
    ADD_INTEGER_GETSET(holder, 17, 0, id);
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
    holder = new TableDataListHolder<QVector, core::Fic>();
    typetableModel = new FicModel();

    SetupTableAccess();


    holder->SetColumns(QStringList() << "fandom" << "author" << "title" << "summary" << "genre" << "characters" << "rated" << "published"
                       << "updated" << "url" << "tags" << "wordCount" << "favourites" << "reviews" << "chapters" << "complete" << "atChapter" << "ID" << "recommendations");

    typetableInterface = QSharedPointer<TableDataInterface>(dynamic_cast<TableDataInterface*>(holder));

    typetableModel->SetInterface(typetableInterface);

    holder->SetData(env.fanfics);
    qwFics = new QQuickWidget();
    QHBoxLayout* lay = new QHBoxLayout;
    lay->addWidget(qwFics);
    ui->wdgFicviewPlaceholder->setLayout(lay);
    qwFics->setResizeMode(QQuickWidget::SizeRootObjectToView);
    qwFics->rootContext()->setContextProperty("ficModel", typetableModel);

    env.interfaces.tags->LoadAlltags();
    tagList = env.interfaces.tags->ReadUserTags();
    qwFics->rootContext()->setContextProperty("tagModel", tagList);
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    qwFics->rootContext()->setContextProperty("urlCopyIconVisible",
                                              settings.value("Settings/urlCopyIconVisible", true).toBool());
    qwFics->rootContext()->setContextProperty("scanIconVisible",
                                              settings.value("Settings/scanIconVisible", true).toBool());
    QUrl source("qrc:/qml/ficview.qml");
    qwFics->setSource(source);

    QObject *childObject = qwFics->rootObject()->findChild<QObject*>("lvFics");

    connect(childObject, SIGNAL(chapterChanged(QVariant, QVariant)), this, SLOT(OnChapterUpdated(QVariant, QVariant)));
    connect(childObject, SIGNAL(tagAdded(QVariant, QVariant)), this, SLOT(OnTagAdd(QVariant,QVariant)));
    connect(childObject, SIGNAL(tagAddedInTagWidget(QVariant, QVariant)), this, SLOT(OnTagAddInTagWidget(QVariant,QVariant)));
    connect(childObject, SIGNAL(tagDeleted(QVariant, QVariant)), this, SLOT(OnTagRemove(QVariant,QVariant)));
    connect(childObject, SIGNAL(tagDeletedInTagWidget(QVariant, QVariant)), this, SLOT(OnTagRemoveInTagWidget(QVariant,QVariant)));
    connect(childObject, SIGNAL(urlCopyClicked(QString)), this, SLOT(OnCopyFicUrl(QString)));
    connect(childObject, SIGNAL(findSimilarClicked(QVariant)), this, SLOT(OnFindSimilarClicked(QVariant)));
    connect(childObject, SIGNAL(recommenderCopyClicked(QString)), this, SLOT(OnOpenRecommenderLinks(QString)));
    connect(childObject, SIGNAL(refilter()), this, SLOT(OnQMLRefilter()));
    connect(childObject, SIGNAL(fandomToggled(QVariant)), this, SLOT(OnQMLFandomToggled(QVariant)));
    QObject* windowObject= qwFics->rootObject();
    connect(windowObject, SIGNAL(backClicked()), this, SLOT(OnDisplayPreviousPage()));
    connect(windowObject, SIGNAL(forwardClicked()), this, SLOT(OnDisplayNextPage()));
    connect(windowObject, SIGNAL(pageRequested(int)), this, SLOT(OnDisplayExactPage(int)));
    ui->deCutoffLimit->setDate(QDateTime::currentDateTime().date());
}

void MainWindow::OnDisplayNextPage()
{
    TaskProgressGuard guard(this);
    QObject* windowObject= qwFics->rootObject();
    windowObject->setProperty("havePagesBefore", true);
    env.filter.recordPage = ++env.pageOfCurrentQuery;
    if(env.sizeOfCurrentQuery <= env.filter.recordLimit * (env.pageOfCurrentQuery))
        windowObject->setProperty("havePagesAfter", false);

    windowObject->setProperty("currentPage", env.pageOfCurrentQuery+1);
    LoadData();
    PlaceResults();
}

void MainWindow::OnDisplayPreviousPage()
{
    TaskProgressGuard guard(this);
    QObject* windowObject= qwFics->rootObject();
    windowObject->setProperty("havePagesAfter", true);
    env.filter.recordPage = --env.pageOfCurrentQuery;

    if(env.pageOfCurrentQuery == 0)
        windowObject->setProperty("havePagesBefore", false);
    windowObject->setProperty("currentPage", env.pageOfCurrentQuery+1);
    LoadData();
    PlaceResults();
}

void MainWindow::OnDisplayExactPage(int page)
{
    TaskProgressGuard guard(this);
    if(page < 0 || page*env.filter.recordLimit > env.sizeOfCurrentQuery)
        return;
    QObject* windowObject= qwFics->rootObject();
    windowObject->setProperty("currentPage", page);
    windowObject->setProperty("havePagesAfter", env.sizeOfCurrentQuery > env.filter.recordLimit * page);
    page--;
    windowObject->setProperty("havePagesBefore", page > 0);


    env.filter.recordPage = page;
    LoadData();
    PlaceResults();
}

MainWindow::~MainWindow()
{
    WriteSettings();
    env.WriteSettings();
    delete ui;
}


void MainWindow::InitInterfaces()
{
    env.InitInterfaces();
}

WebPage MainWindow::RequestPage(QString pageUrl, ECacheMode cacheMode, bool autoSaveToDB)
{
    QString toInsert = "<a href=\"" + pageUrl + "\"> %1 </a>";
    toInsert= toInsert.arg(pageUrl);
    ui->edtResults->append("<span>Processing url: </span>");
    if(toInsert.trimmed().isEmpty())
        toInsert=toInsert;
    ui->edtResults->insertHtml(toInsert);

    pbMain->setTextVisible(false);
    pbMain->show();

    return env::RequestPage(pageUrl, cacheMode, autoSaveToDB);
}


void MainWindow::LoadData()
{
    if(ui->cbMinWordCount->currentText().trimmed().isEmpty())
        ui->cbMinWordCount->setCurrentText("0");

    if(env.filter.recordPage == 0)
    {
        env.sizeOfCurrentQuery = GetResultCount();
        QObject* windowObject= qwFics->rootObject();
        int currentActuaLimit = ui->chkRandomizeSelection->isChecked() ? ui->sbMaxRandomFicCount->value() : env.filter.recordLimit;
        windowObject->setProperty("totalPages", env.filter.recordLimit > 0 ? (env.sizeOfCurrentQuery/currentActuaLimit) + 1 : 1);
        windowObject->setProperty("currentPage", env.filter.recordLimit > 0 ? env.filter.recordPage+1 : 1);
        windowObject->setProperty("havePagesBefore", false);
        windowObject->setProperty("havePagesAfter", env.filter.recordLimit > 0 && env.sizeOfCurrentQuery > env.filter.recordLimit);

    }
    //ui->edtResults->setOpenExternalLinks(true);
    //ui->edtResults->clear();
    //ui->edtResults->setUpdatesEnabled(false);

    env.LoadData();
    holder->SetData(env.fanfics);
}

int MainWindow::GetResultCount()
{
    // for random walking size doesn't mater
    if(ui->chkRandomizeSelection->isChecked())
        return ui->sbMaxRandomFicCount->value()-1;


    return env.GetResultCount();
}



void MainWindow::ProcessTagsIntoGui()
{
    auto tagList = env.interfaces.tags->ReadUserTags();
    QList<QPair<QString, QString>> tagPairs;

    for(auto tag : tagList)
        tagPairs.push_back({ "0", tag });
    ui->wdgTagsPlaceholder->InitFromTags(-1, tagPairs);

}

void MainWindow::SetTag(int id, QString tag, bool silent)
{
    env.interfaces.tags->SetTagForFic(id, tag);
    tagList = env.interfaces.tags->ReadUserTags();
}

void MainWindow::UnsetTag(int id, QString tag)
{
    env.interfaces.tags->RemoveTagFromFic(id, tag);
    tagList = env.interfaces.tags->ReadUserTags();
}

void MainWindow::LoadMoreAuthors()
{
    TaskProgressGuard guard(this);
    QString listName = ui->cbRecGroup->currentText();
    auto cacheMode = ui->chkWaveOnlyCache->isChecked() ? ECacheMode::use_only_cache : ECacheMode::dont_use_cache;
    DisableAllLoadButtons();

    env.LoadMoreAuthors(listName, cacheMode);

    ui->edtResults->clear();
    AddToProgressLog(" Found recommenders: ");
    ShutdownProgressbar();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    EnableAllLoadButtons();
}

void MainWindow::UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater)
{
    TaskProgressGuard guard(this);
    env.filter.mode = core::StoryFilter::filtering_in_recommendations;
    DisableAllLoadButtons();

    env.ReprocessAuthorNamesFromTheirPages();

    ShutdownProgressbar();
    ui->edtResults->clear();
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    EnableAllLoadButtons();
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

void MainWindow::OnCopyFicUrl(QString text)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
    ui->edtResults->insertPlainText(text + "\n");

}

void MainWindow::OnOpenRecommenderLinks(QString url)
{
    if(!ui->chkHeartProfile->isChecked())
        return;
    auto webId = url_utils::GetWebId(url, "ffn");
    auto id = env.interfaces.fanfics->GetIDFromWebID(webId.toInt(), "ffn");
    auto recommenders = env.interfaces.recs->GetRecommendersForFicId(id);

    for(auto recommender : recommenders)
    {
        auto author = env.interfaces.authors->GetById(recommender);
        if(!author || !env.interfaces.recs->GetMatchCountForRecommenderOnList(author->id, env.interfaces.recs->GetCurrentRecommendationList()))
            continue;
        QDesktopServices::openUrl(author->url("ffn"));
    }
}

void MainWindow::OnCopyAllUrls()
{
    TaskProgressGuard guard(this);
    QClipboard *clipboard = QApplication::clipboard();
    QString result;
    for(int i = 0; i < typetableModel->rowCount(); i ++)
    {
        if(ui->chkInfoForLinks->isChecked())
        {
            result += typetableModel->index(i, 2).data().toString() + "\n";
        }
        result += "https://www.fanfiction.net/s/" + typetableModel->index(i, 9).data().toString() + "\n";//\n
    }
    clipboard->setText(result);
}

void MainWindow::OnDoFormattedListByFandoms()
{
    TaskProgressGuard guard(this);
    QClipboard *clipboard = QApplication::clipboard();
    QString result;
    QList<int> ficIds;
    for(int i = 0; i < typetableModel->rowCount(); i ++)
        ficIds.push_back(typetableModel->index(i, 17).data().toInt());
    QSet<QPair<QString, int>> already;
    QMap<int, QList<core::Fic*>> byFandoms;
    for(core::Fic& fic : env.fanfics)
    {
        auto* ficPtr = &fic;

        auto fandoms = ficPtr->fandomIds;
        if(fandoms.size() == 0)
        {
            auto fandom = ficPtr->fandom.trimmed();
            qDebug() << "no fandoms written for: " << "https://www.fanfiction.net/s/" + QString::number(ficPtr->webId) + ">";
        }
        for(auto fandom: fandoms)
        {
            byFandoms[fandom].push_back(ficPtr);
        }
    }
    QHash<int, QString> fandomnNames;
    fandomnNames = env.interfaces.fandoms->GetFandomNamesForIDs(byFandoms.keys());

    result += "<ul>";
    for(auto fandomKey : byFandoms.keys())
    {
        QString name = fandomnNames[fandomKey];
        result+= "<li><a href=\"#" + name.toLower().replace(" ","_") +"\">" + name + "</a></li>";
    }
    An<PageManager> pager;
    pager->SetDatabase(QSqlDatabase::database());
    for(auto fandomKey : byFandoms.keys())
    {
        QString name = fandomnNames[fandomKey];
        result+="<h4 id=\""+ name.toLower().replace(" ","_") +  "\">" + name + "</h4>";

        for(auto fic : byFandoms[fandomKey])
        {
            auto* ficPtr = fic;
            QPair<QString, int> key = {name, fic->id};

            if(already.contains(key))
                continue;
            already.insert(key);

            auto genreString = ficPtr->genreString;
            bool validGenre = true;
            if(validGenre)
            {
                result+="<a href=https://www.fanfiction.net/s/" + QString::number(ficPtr->webId) + ">" + ficPtr->title + "</a> by " + ficPtr->author->name + "<br>";
                result+=ficPtr->genreString + "<br><br>";
                QString status = "<b>Status:</b> <font color=\"%1\">%2</font>";

                if(ficPtr->complete)
                    result+=status.arg("green").arg("Complete<br>");
                else
                    result+=status.arg("red").arg("Active<br>");
                result+=ficPtr->summary + "<br><br>";
            }

        }
    }
    clipboard->setText(result);
}

void MainWindow::OnDoFormattedList()
{
    TaskProgressGuard guard(this);
    QClipboard *clipboard = QApplication::clipboard();
    QString result;
    QList<int> ficIds;
    for(int i = 0; i < typetableModel->rowCount(); i ++)
        ficIds.push_back(typetableModel->index(i, 17).data().toInt());
    QSet<QPair<QString, int>> already;
    QMap<QString, QList<int>> byFandoms;
    for(core::Fic& fic : env.fanfics)
    {
        //auto ficPtr = env.interfaces.fanfics->GetFicById(id);
        auto* ficPtr = &fic;
        auto genreString = ficPtr->genreString;
        bool validGenre = true;
        if(validGenre)
        {
            result+="<a href=https://www.fanfiction.net/s/" + QString::number(ficPtr->webId) + ">" + ficPtr->title + "</a> by " + ficPtr->author->name + "<br>";
            result+=ficPtr->genreString + "<br><br>";
            QString status = "<b>Status:</b> <font color=\"%1\">%2</font>";

            if(ficPtr->complete)
                result+=status.arg("green").arg("Complete<br>");
            else
                result+=status.arg("red").arg("Active<br>");
            result+=ficPtr->summary + "<br><br>";
        }
    }
    clipboard->setText(result);
}

void MainWindow::on_pbLoadDatabase_clicked()
{
    TaskProgressGuard guard(this);
    env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_fics);
    env.filter.recordPage = 0;
    env.pageOfCurrentQuery = 0;
    if(env.filter.isValid)
    {
        LoadData();
        PlaceResults();
    }
}

void MainWindow::LoadAutomaticSettingsForRecListSources(int size)
{
    if(size == 0)
        return;
    if(size == 1)
    {
        ui->leRecsMinimumMatches->setText("1");
        ui->leRecsPickRatio->setText("99999");
        ui->leRecsAlwaysPickAt->setText("1");
        return;
    }
    if(size > 1 && size <= 7)
    {
        ui->leRecsMinimumMatches->setText("4");
        ui->leRecsPickRatio->setText("70");
        ui->leRecsAlwaysPickAt->setText(QString::number(size));
        return;
    }
    if(size > 7 && size <= 20)
    {
        ui->leRecsMinimumMatches->setText("6");
        ui->leRecsPickRatio->setText("70");
        ui->leRecsAlwaysPickAt->setText(QString::number(size));
        return;
    }
    if(size > 20 && size <= 300)
    {
        ui->leRecsMinimumMatches->setText("6");
        ui->leRecsPickRatio->setText("50");
        ui->leRecsAlwaysPickAt->setText(QString::number(size));
        return;
    }
    if(size > 300)
    {
        ui->leRecsMinimumMatches->setText("6");
        ui->leRecsPickRatio->setText("40");
        ui->leRecsAlwaysPickAt->setText(QString::number(size));
        return;
    }
}

QList<QSharedPointer<core::Fic>> MainWindow::LoadFavourteLinksFromFFNProfile(QString url)
{
    QList<QSharedPointer<core::Fic>> result;
    //need to make sure it *is* FFN url
    //https://www.fanfiction.net/u/3697775/Rumour-of-an-Alchemist
    QRegularExpression rx("https://www.fanfiction.net/u/(\\d)+");
    auto match = rx.match(url);
    if(!match.hasMatch())
    {
        QMessageBox::warning(nullptr, "Warning!", "URL is not an FFN author url\nNeeeds to be a https://www.fanfiction.net/u/NUMERIC_ID");
        return result;
    }
    result = env.LoadAuthorFics(url);
    return result;
}

void MainWindow::OnQMLRefilter()
{
    on_pbLoadDatabase_clicked();
}

void MainWindow::OnQMLFandomToggled(QVariant var)
{
    auto fanficsInterface = env.interfaces.fanfics;

    int rownum = var.toInt();

    QModelIndex index = typetableModel->index(rownum, 0);
    auto data = index.data(static_cast<int>(FicModel::FandomRole)).toString();

    QStringList list = data.split("&", QString::SkipEmptyParts);
    if(!list.size())
        return;
    if(ui->cbIgnoreFandomSelector->currentText().trimmed() != list.at(0).trimmed())
       ui->cbIgnoreFandomSelector->setCurrentText(list.at(0).trimmed());
    else if(list.size() > 1)
        ui->cbIgnoreFandomSelector->setCurrentText(list.at(1).trimmed());
}


void MainWindow::ReadSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    ui->chkShowDirectRecs->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->pbReprocessAuthors->setVisible(settings.value("Settings/showListBuildButton", false).toBool());
    ui->wdgWave->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->wdgCustomActions->setVisible(settings.value("Settings/showCustomActions", false).toBool());
    ui->pbLoadAllRecommenders->setVisible(settings.value("Settings/showRecListReload", false).toBool());
    ui->sbMinRecommendations->setVisible(settings.value("Settings/showRecListReload", false).toBool());
    ui->chkShowOrigins->setVisible(settings.value("Settings/showOriginsCheck", false).toBool());
    ui->label_16->setVisible(settings.value("Settings/showRecListReload", false).toBool());
    ui->wdgOpenID->setVisible(settings.value("Settings/showOpenID", false).toBool());


    ui->chkGroupFandoms->setVisible(settings.value("Settings/showListCreation", false).toBool());
    bool thinClient = settings.value("Settings/thinClient").toBool();
    if(thinClient)
        ui->pbFormattedList->setVisible(settings.value("Settings/showHTLProto", false).toBool());
    ui->chkInfoForLinks->setVisible(settings.value("Settings/showListCreation", false).toBool());
    ui->pbFirstWave->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    ui->pbWipeFandom->setVisible(settings.value("Settings/pbWipeFandom", false).toBool());
    ui->chkCacheMode->setVisible(settings.value("Settings/showCacheMode", false).toBool());
    ui->cbCustomFilters->setVisible(settings.value("Settings/showCustomFilters", false).toBool());
    ui->chkCustomFilter->setVisible(settings.value("Settings/showCustomFilters", false).toBool());

    {
        ui->pbCreateNewList->setVisible(settings.value("Settings/showRecListCustomization", false).toBool());
        ui->label_18->setVisible(settings.value("Settings/showRecListCustomization", false).toBool());
        ui->leCurrentListName->setVisible(settings.value("Settings/showRecListCustomization", false).toBool());
        ui->chkSyncListNameToView->setVisible(settings.value("Settings/showRecListCustomization", false).toBool());
        ui->pbRemoveList->setVisible(settings.value("Settings/showRecListCustomization", false).toBool());
    }

    ui->cbRecGroup->setVisible(settings.value("Settings/showRecListSelector", false).toBool());
    ui->chkTrackedFandom->setVisible(settings.value("Settings/showTracking", false).toBool());
    ui->pbPauseTask->setVisible(settings.value("Settings/showTaskButtons", false).toBool());
    ui->pbContinueTask->setVisible(settings.value("Settings/showTaskButtons", false).toBool());
    ui->pbLoadTrackedFandoms->setVisible(settings.value("Settings/showTracking", false).toBool());

    ui->cbNormals->setCurrentText(settings.value("Settings/normals", "").toString());

    ui->chkTrackedFandom->blockSignals(true);
    auto fandomName = GetCurrentFandomName();
    auto fandom = env.interfaces.fandoms->GetFandom(fandomName);
    if(fandom)
        ui->chkTrackedFandom->setChecked(fandom->tracked);
    else
        ui->chkTrackedFandom->setChecked(false);
    ui->chkTrackedFandom->blockSignals(false);

    ui->cbMaxWordCount->setCurrentText(settings.value("Settings/maxWordCount", "").toString());
    ui->cbMinWordCount->setCurrentText(settings.value("Settings/minWordCount", 100000).toString());

    ui->leContainsGenre->setText(settings.value("Settings/plusGenre", "").toString());
    ui->leNotContainsGenre->setText(settings.value("Settings/minusGenre", "").toString());
    ui->leNotContainsWords->setText(settings.value("Settings/minusWords", "").toString());
    ui->leContainsWords->setText(settings.value("Settings/plusWords", "").toString());

    ui->chkHeartProfile->setChecked(settings.value("Settings/chkHeartProfile", false).toBool());
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
    //ui->chkShowRecsRegardlessOfTags->setChecked(settings.value("Settings/ignoreTagsOnRecommendations", false).toBool());
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
    settings.setValue("Settings/normals", GetCurrentFandomName());
    settings.setValue("Settings/plusGenre", ui->leContainsGenre->text());
    settings.setValue("Settings/minusGenre", ui->leNotContainsGenre->text());
    settings.setValue("Settings/plusWords", ui->leContainsWords->text());
    settings.setValue("Settings/minusWords", ui->leNotContainsWords->text());


    settings.setValue("Settings/chkHeartProfile", ui->chkHeartProfile->isChecked());
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
    settings.setValue("Settings/customFilterEnabled", ui->chkCustomFilter->isChecked());
    settings.setValue("Settings/biasMode", ui->cbBiasFavor->currentText());
    settings.setValue("Settings/biasOperator", ui->cbBiasOperator->currentText());
    settings.setValue("Settings/biasValue", ui->leBiasValue->text());
    settings.setValue("Settings/lengthCutoff", ui->cbWordCutoff->currentText());
    settings.setValue("Settings/currentList", ui->cbRecGroup->currentText());
    settings.sync();
}

QString MainWindow::GetCurrentFandomName()
{
    return core::Fandom::ConvertName(ui->cbNormals->currentText());
}

int MainWindow::GetCurrentFandomID()
{
    return env.interfaces.fandoms->GetIDForName(core::Fandom::ConvertName(ui->cbNormals->currentText()));
    //return core::Fandom::ConvertName(ui->cbNormals->currentText());
}

void MainWindow::OnChapterUpdated(QVariant id, QVariant chapter)
{
    env.interfaces.fanfics->AssignChapter(id.toInt(), chapter.toInt());
}


void MainWindow::OnTagAdd(QVariant tag, QVariant row)
{
    int rownum = row.toInt();
    QModelIndex index = typetableModel->index(rownum, 10);
    auto data = typetableModel->data(index, 0).toString();
    data += " " + tag.toString();

    auto id = typetableModel->data(typetableModel->index(rownum, 17), 0).toInt();
    SetTag(id, tag.toString());

    typetableModel->setData(index,data,0);
    typetableModel->updateAll();
}

void MainWindow::OnTagRemove(QVariant tag, QVariant row)
{
    int rownum = row.toInt();
    QModelIndex index = typetableModel->index(row.toInt(), 10);
    auto data = typetableModel->data(index, 0).toString();
    data = data.replace(tag.toString(), "");

    auto id = typetableModel->data(typetableModel->index(rownum, 17), 0).toInt();
    UnsetTag(id, tag.toString());

    typetableModel->setData(index,data,0);
    typetableModel->updateAll();
}

void MainWindow::OnTagAddInTagWidget(QVariant tag, QVariant row)
{
    int rownum = row.toInt();
    SetTag(rownum, tag.toString());
}

void MainWindow::OnTagRemoveInTagWidget(QVariant tag, QVariant row)
{
    int rownum = row.toInt();
    UnsetTag(rownum, tag.toString());
}


void MainWindow::ReinitRecent(QString name)
{
    env.interfaces.fandoms->PushFandomToTopOfRecent(name);
    env.interfaces.fandoms->ReloadRecentFandoms();
    recentFandomsModel->setStringList(env.interfaces.fandoms->GetRecentFandoms());
}

void MainWindow::StartTaskTimer()
{
    taskTimer.setSingleShot(true);
    taskTimer.start(1000);
}


// this has too much gui code to be properly separated
// creating a split function just for server instead
void MainWindow::CheckUnfinishedTasks()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    if(settings.value("Settings/skipUnfinishedTasksCheck",true).toBool())
        return;
    auto tasks = env.interfaces.pageTask->GetUnfinishedTasks();
    TaskList tasksToResume;
    for(auto task : tasks)
    {
        QString diagnostics;
        diagnostics+= "Unfinished task:\n";
        diagnostics+= task->taskComment + "\n";
        diagnostics+= "Started: " + task->startedAt.toString("yyyyMMdd hh:mm") + "\n";
        diagnostics+= "Do you want to continue this task?";
        QMessageBox m; /*(QMessageBox::Warning, "Unfinished task warning",
                      diagnostics,QMessageBox::Ok|QMessageBox::Cancel);*/
        m.setIcon(QMessageBox::Warning);
        m.setText(diagnostics);
        auto continueTask =  m.addButton("Continue",QMessageBox::AcceptRole);
        auto dropTask =      m.addButton("Drop task",QMessageBox::AcceptRole);
        auto delayDecision = m.addButton("Ask next time",QMessageBox::AcceptRole);
        Q_UNUSED(delayDecision);
        m.exec();
        if(m.clickedButton() == dropTask)
            env.interfaces.pageTask->DropTaskId(task->id);
        else if(m.clickedButton() == continueTask)
            tasksToResume.push_back(task);
        else
        {
            //do nothing with this task. it will appear on next application start
        }
    }

    // later this needs to be preceded by code that routes tasks based on their type.
    // hard coding for now to make sure base functionality works
    {
        TaskProgressGuard guard(this);
        for(auto task : tasksToResume)
        {
            auto fullTask = env.interfaces.pageTask->GetTaskById(task->id);
            if(fullTask->type == 0)
            {
                InitUIFromTask(fullTask);
                UseAuthorTask(fullTask);
            }
            else
            {
                if(!task)
                    return;
                ReinitProgressbar(fullTask->size);
                DisableAllLoadButtons();
                env.UseFandomTask(fullTask);
                thread_local QString status = "<font color=\"%2\"><b>%1:</b> </font>%3<br>";
                AddToProgressLog("Finished the job <br>");
                ui->edtResults->insertHtml(status.arg("Inserted fics").arg("darkGreen").arg(fullTask->addedFics));
                ui->edtResults->insertHtml(status.arg("Updated fics").arg("darkBlue").arg(fullTask->updatedFics));
                ui->edtResults->insertHtml(status.arg("Duplicate fics").arg("gray").arg(fullTask->skippedFics));
            }
        }
    }
}

bool MainWindow::AskYesNoQuestion(QString value)
{
    QMessageBox m;
    m.setIcon(QMessageBox::Warning);
    m.setText(value);
    auto yesButton =  m.addButton("Yes", QMessageBox::AcceptRole);
    auto noButton =  m.addButton("Cancel", QMessageBox::AcceptRole);
    Q_UNUSED(noButton);
    m.exec();
    if(m.clickedButton() != yesButton)
        return false;
    return true;
}

void MainWindow::PlaceResults()
{
    ui->edtResults->setUpdatesEnabled(true);
    ui->edtResults->setReadOnly(true);
    holder->SetData(env.fanfics);
    ReinitRecent(GetCurrentFandomName());
}

void MainWindow::SetWorkingStatus()
{
    refreshSpin.setScaledSize(actionProgress->ui->lblCurrentStatusIcon->size());
    refreshSpin.start();
    actionProgress->ui->lblCurrentStatusIcon->setMovie(&refreshSpin);
    actionProgress->ui->lblCurrentStatus->setText("working...");
}

void MainWindow::SetFinishedStatus()
{
    QPixmap px(":/icons/icons/ok.png");
    px = px.scaled(24, 24);
    actionProgress->ui->lblCurrentStatusIcon->setPixmap(px);
    actionProgress->ui->lblCurrentStatus->setText("Done!");
}

void MainWindow::SetFailureStatus()
{
    QPixmap px(":/icons/icons/error2.png");
    px = px.scaled(24, 24);
    actionProgress->ui->lblCurrentStatusIcon->setPixmap(px);
    actionProgress->ui->lblCurrentStatus->setText("Error!");
}

void MainWindow::on_pbCrawl_clicked()
{
    CrawlFandom(GetCurrentFandomName());
}


void MainWindow::OnTagToggled(QString , bool )
{
    if(ui->wdgTagsPlaceholder->GetSelectedTags().size() == 0)
        ui->chkEnableTagsFilter->setChecked(false);
    else
        ui->chkEnableTagsFilter->setChecked(true);
}

// needed to bind values to reasonable limits
void MainWindow::on_chkRandomizeSelection_clicked(bool checked)
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    auto ficLimitActive =  ui->chkRandomizeSelection->isChecked();
    int maxFicCountValue = ficLimitActive ? ui->sbMaxRandomFicCount->value()  : 0;
    if(checked && (maxFicCountValue < 1 || maxFicCountValue >50))
        ui->sbMaxRandomFicCount->setValue(settings.value("Settings/defaultRandomFicCount", 6).toInt());
}


void MainWindow::on_cbSortMode_currentTextChanged(const QString &)
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(ui->cbSortMode->currentText() == "Rec Count")
        ui->cbRecGroup->setVisible(settings.value("Settings/showRecListSelector", false).toBool());
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

void MainWindow::OnNewSelectionInRecentList(const QModelIndex &current, const QModelIndex &)
{
    ui->cbNormals->setCurrentText(current.data().toString());
    ui->chkTrackedFandom->blockSignals(true);
    auto fandom = env.interfaces.fandoms->GetFandom(GetCurrentFandomName());
    if(fandom)
        ui->chkTrackedFandom->setChecked(fandom->tracked);

    ui->chkTrackedFandom->blockSignals(false);
}

void MainWindow::OnNewSelectionInRecommenderList(const QModelIndex &current, const QModelIndex &)
{
    QString recommender = current.data().toString();
    auto author = env.interfaces.authors->GetAuthorByNameAndWebsite(recommender, "ffn");
    if(author)
    {
        ui->leAuthorUrl->setText(author->url("ffn"));
        ui->cbAuthorNames->setCurrentText(author->name);
    }

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



//void MainWindow::CreatePageThreadWorker()
//{
//    env.worker = new PageThreadWorker;
//    env.worker->moveToThread(&env.pageThread);

//}

//void MainWindow::StartPageWorker()
//{
//    env.pageQueue.data.clear();
//    env.pageQueue.pending = true;
//    //worker->SetAutomaticCache(QDate::currentDate());
//    env.pageThread.start(QThread::HighPriority);

//}

//void MainWindow::StopPageWorker()
//{
//    env.pageThread.quit();
//}

void MainWindow::ReinitProgressbar(int maxValue)
{
    pbMain->setMaximum(maxValue);
    pbMain->setValue(0);
    pbMain->show();
}

void MainWindow::ShutdownProgressbar()
{
    pbMain->setValue(0);
    //pbMain->hide();
}

void MainWindow::AddToProgressLog(QString value)
{
    ui->edtResults->insertHtml(value);
    ui->edtResults->ensureCursorVisible();
    QCoreApplication::processEvents();
}


void MainWindow::FillRecTagCombobox()
{
    auto lists = env.interfaces.recs->GetAllRecommendationListNames();
    SilentCall(ui->cbRecGroup)->setModel(new QStringListModel(lists));

    QSettings settings("settings.ini", QSettings::IniFormat);
    auto storedRecList = settings.value("Settings/currentList").toString();
    qDebug() << QDir::currentPath();
    ui->cbRecGroup->setCurrentText(storedRecList);
}

void MainWindow::FillRecommenderListView(bool forceRefresh)
{
    QStringList result;
    auto list = env.interfaces.recs->GetCurrentRecommendationList();
    auto allStats = env.interfaces.recs->GetAuthorStatsForList(list, forceRefresh);
    std::sort(std::begin(allStats),std::end(allStats), [](auto s1, auto s2){
        return s1->matchRatio < s2->matchRatio;
    });
    for(auto stat : allStats)
        result.push_back(stat->authorName);
    recommendersModel->setStringList(result);
    ui->lvRecommenders->setModel(recommendersModel);
    ui->cbAuthorNames->setModel(recommendersModel);
}

core::AuthorPtr MainWindow::LoadAuthor(QString url)
{
    TaskProgressGuard guard(this);
    return env.LoadAuthor(url, QSqlDatabase::database());
}

void MainWindow::on_chkTrackedFandom_toggled(bool checked)
{
    env.interfaces.fandoms->SetTracked(GetCurrentFandomName(),checked);
}


bool MainWindow::WarnCutoffLimit()
{
    if(ui->chkCutoffLimit->isChecked())
    {
        QString diagnostics;
        diagnostics = "Date cutoff is enabled.\n";
        diagnostics += "Fandoms will be updated up to that date instead of last update.\n";
        diagnostics += "Unless their update date is older than this date.";
        if(!AskYesNoQuestion(diagnostics))
            return false;
    }
    return true;
}

bool MainWindow::WarnFullParse()
{
    if(ui->chkIgnoreUpdateDate->isChecked())
    {
        QString diagnostics;
        diagnostics = "Update date is fully ignored.\n";
        diagnostics += "Fandoms will be parsed up to the very beginning of their history.\n";
        diagnostics += "This is potentially a very long process for big fandoms.";
        if(!AskYesNoQuestion(diagnostics))
            return false;
    }
    return true;
}

PageTaskPtr MainWindow::ProcessFandomsAsTask(QList<core::FandomPtr> fandoms, QString taskComment, bool allowCacheRefresh)
{
    ForcedFandomUpdateDate forcedDate = CreateForcedUpdateDateFromGUI();
    auto result = env.ProcessFandomsAsTask(fandoms,
                                           taskComment,
                                           allowCacheRefresh,
                                           GetCurrentCacheMode(),
                                           ui->cbWordCutoff->currentText(),
                                           forcedDate);
    return result;
}

void MainWindow::CrawlFandom(QString fandomName)
{
    if(!WarnCutoffLimit() || !WarnFullParse())
        return;

    TaskProgressGuard guard(this);
    DisableAllLoadButtons();
    auto fandom = env.interfaces.fandoms->GetFandom(fandomName);
    if(!fandom)
        return;

    auto urls = fandom->GetUrls();
    ui->edtResults->clear();

    auto task = ProcessFandomsAsTask({fandom}, "Loading the fandom: " + fandomName, true);

    QString status = "<font color=\"%2\"><b>%1:</b> </font>%3<br>";
    AddToProgressLog("Finished the job <br>");
    ui->edtResults->insertHtml(status.arg("Inserted fics").arg("darkGreen").arg(task->addedFics));
    ui->edtResults->insertHtml(status.arg("Updated fics").arg("darkBlue").arg(task->updatedFics));
    ui->edtResults->insertHtml(status.arg("Duplicate fics").arg("gray").arg(task->skippedFics));

    status = "Finished processing %1 fics";

    ReinitRecent(fandom->GetName());
    ui->lvTrackedFandoms->setModel(recentFandomsModel);
    EnableAllLoadButtons();
}

ECacheMode MainWindow::GetCurrentCacheMode() const
{
    ECacheMode result;

    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    //ui->chkShowDirectRecs->setVisible(settings.value("Settings/showExperimentaWaveparser", false).toBool());
    if(settings.value("Settings/releaseCacheMode", false).toBool())
        result = ECacheMode::use_cache;
    else
        result = ui->chkCacheMode->isChecked() ? ECacheMode::use_cache : ECacheMode::dont_use_cache;
    return result;
}

void MainWindow::CreateSimilarListForGivenFic(int id)
{
    TaskProgressGuard guard(this);
    QSqlDatabase db = QSqlDatabase::database();
    env.CreateSimilarListForGivenFic(id, db);
    ui->cbNormals->setCurrentText("");
    OpenRecommendationList("similar");
}


void MainWindow::ReprocessAllAuthorsV2()
{
    TaskProgressGuard guard(this);
    if(env.filter.isValid)
    {
        env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations);

        QSqlDatabase db = QSqlDatabase::database();
        AuthorStatsProcessor authorProcessor(db,
                                             env.interfaces.fanfics,
                                             env.interfaces.fandoms,
                                             env.interfaces.authors);
        authorProcessor.ReprocessAllAuthorsStats(ui->chkWaveOnlyCache->isChecked() ? ECacheMode::use_cache : ECacheMode::dont_use_cache);
    }
}

void MainWindow::ReprocessAllAuthorsJustSlash(QString fieldUsed)
{
    env.interfaces.authors->CalculateSlashStatisticsPercentages(fieldUsed);
}

void MainWindow::DetectSlashForEverythingV2()
{
    TaskProgressGuard guard(this);
    QSqlDatabase db = QSqlDatabase::database();
    SlashProcessor slashProcessor(db,
                                  env.interfaces.fanfics,
                                  env.interfaces.fandoms,
                                  env.interfaces.pageTask,
                                  env.interfaces.authors,
                                  env.interfaces.recs,
                                  env.interfaces.db);
    slashProcessor.AssignSlashKeywordsMetaInfomation(db);
}

void MainWindow::DoFullCycle()
{
    QSqlDatabase db = QSqlDatabase::database();
    SlashProcessor slashProcessor(db,
                                  env.interfaces.fanfics,
                                  env.interfaces.fandoms,
                                  env.interfaces.pageTask,
                                  env.interfaces.authors,
                                  env.interfaces.recs,
                                  env.interfaces.db);
    slashProcessor.DoFullCycle(db, ui->sbSlashPasses->value());
}

void MainWindow::UseAuthorTask(PageTaskPtr task)
{
    env.UseAuthorTask(task);
}

ForcedFandomUpdateDate MainWindow::CreateForcedUpdateDateFromGUI()
{
    ForcedFandomUpdateDate forcedDate;
    if(ui->chkCutoffLimit->isChecked())
    {
        forcedDate.isValid = true;
        forcedDate.date = ui->deCutoffLimit->date();
    }
    if(ui->chkIgnoreUpdateDate->isChecked())
    {
        forcedDate.isValid = true;
        forcedDate.date = QDate();
    }
    return forcedDate;
}

void MainWindow::SetClientMode()
{
    ui->pbReinitFandoms->hide();
    ui->pbWipeFandom->hide();
    ui->widget_4->hide();
    ui->chkTrackedFandom->hide();
    ui->wdgAdminActions->hide();
    ui->chkCustomFilter->hide();
    ui->cbCustomFilters->hide();
    ui->chkHeartProfile->setChecked(false);
    ui->chkHeartProfile->setVisible(false);
    ui->tabWidget->removeTab(2);
    ui->tabWidget->removeTab(1);
    ui->chkLimitPageSize->setChecked(true);
    ui->chkLimitPageSize->setEnabled(false);

}

void MainWindow::on_pbLoadTrackedFandoms_clicked()
{
    if(!WarnCutoffLimit() || !WarnFullParse())
        return;

    QString diagnostics;
    diagnostics = "You are initiating an update of all tracked fandoms.\n";
    diagnostics += "This is potentially a very long operation.\n";
    diagnostics += "Are you sure?";
    if(!AskYesNoQuestion(diagnostics))
        return;

    TaskProgressGuard guard(this);
    auto task = env.LoadTrackedFandoms(CreateForcedUpdateDateFromGUI(),
                                       GetCurrentCacheMode(),
                                       ui->cbWordCutoff->currentText());

    QString status = "<font color=\"%2\"><b>%1:</b> </font>%3<br>";
    AddToProgressLog("Finished the job <br>");
    ui->edtResults->insertHtml(status.arg("Inserted fics").arg("darkGreen").arg(task->addedFics));
    ui->edtResults->insertHtml(status.arg("Updated fics").arg("darkBlue").arg(task->updatedFics));
    ui->edtResults->insertHtml(status.arg("Duplicate fics").arg("gray").arg(task->skippedFics));

    status = "Finished processing %1 fics";
}

void MainWindow::on_pbLoadPage_clicked()
{
    TaskProgressGuard guard(this);

    env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations, true);
    if(env.filter.isValid)
    {
        //ui->leAuthorUrl->text()

        TimedAction action("Full open",[&](){
            env.LoadAuthor(ui->leAuthorUrl->text(), QSqlDatabase::database());
        });
        action.run();
        LoadData();
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(env.fanfics);
    }
}


void MainWindow::on_pbOpenRecommendations_clicked()
{
    TaskProgressGuard guard(this);
    env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations, true);
    if(env.filter.isValid)
    {
        auto startRecLoad = std::chrono::high_resolution_clock::now();
        LoadData();
        auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;

        qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(env.fanfics);
    }
}

void MainWindow::on_pbLoadAllRecommenders_clicked()
{
    TaskProgressGuard guard(this);
    env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_in_recommendations);
    if(env.filter.isValid)
    {
        QSqlDatabase db = QSqlDatabase::database();
        database::Transaction transaction(db);

        pbMain->setValue(0);
        pbMain->setTextVisible(true);
        pbMain->setFormat("%v");

        RecommendationsProcessor reloader(db, env.interfaces.fanfics,
                                          env.interfaces.fandoms,
                                          env.interfaces.authors,
                                          env.interfaces.recs);

        connect(&reloader, &RecommendationsProcessor::resetEditorText, this, &MainWindow::OnResetTextEditor);
        connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &MainWindow::OnProgressBarRequested);
        connect(&reloader, &RecommendationsProcessor::updateCounter, this, &MainWindow::OnUpdatedProgressValue);
        connect(&reloader, &RecommendationsProcessor::updateInfo, this, &MainWindow::OnNewProgressString);
        reloader.ReloadRecommendationsList(ui->leAuthorUrl->text(), GetCurrentCacheMode());

        ui->leAuthorUrl->setText("");
        auto startRecLoad = std::chrono::high_resolution_clock::now();
        LoadData();
        auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
        qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(env.fanfics);
    }
}

void MainWindow::on_pbOpenWholeList_clicked()
{
    env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_whole_list,
                                           false,
                                           ui->cbRecGroup->currentText());
    if(env.filter.isValid)
    {
        ui->leAuthorUrl->setText("");
        auto startRecLoad = std::chrono::high_resolution_clock::now();
        LoadData();
        auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
        qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(env.fanfics);
    }
}

void MainWindow::OpenRecommendationList(QString listName)
{
    env.filter = ProcessGUIIntoStoryFilter(core::StoryFilter::filtering_whole_list,
                                           false,
                                           listName);
    if(env.filter.isValid)
    {
        env.filter.sortMode = core::StoryFilter::sm_reccount;

        ui->leAuthorUrl->setText("");
        auto startRecLoad = std::chrono::high_resolution_clock::now();
        LoadData();
        auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
        qDebug() << "Loaded recs in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ui->edtResults->setUpdatesEnabled(true);
        ui->edtResults->setReadOnly(true);
        holder->SetData(env.fanfics);
    }
}

void MainWindow::on_pbFirstWave_clicked()
{
    LoadMoreAuthors();
}



int MainWindow::BuildRecommendations(QSharedPointer<core::RecommendationList> params, bool clearAuthors)
{
    TaskProgressGuard guard(this);
    auto result = env.BuildRecommendations(params, env.GetSourceFicsFromFile("lists/source.txt"), clearAuthors);
    if(result == -1)
    {
        QMessageBox::warning(nullptr, "Attention!", "Could not create a list with such parameters\n"
                                                    "Try using more source fics, or loosen the restrictions");
    }

    FillRecTagCombobox();
    FillRecommenderListView();

    return result;
}


FilterErrors MainWindow::ValidateFilter()
{
    FilterErrors result;

    bool wordSearch = ui->chkWordsPlus->isChecked() && !ui->leContainsWords->text().isEmpty();
    wordSearch = wordSearch || (ui->chkWordsMinus->isChecked() && !ui->leNotContainsWords->text().isEmpty());
    QString minWordCount= ui->cbMinWordCount->currentText();
    if(wordSearch && (ui->cbNormals->currentText().trimmed().isEmpty() && minWordCount.toInt() < 60000))
    {
        result.AddError("Word search is currently only possible if:");
        result.AddError("    1) A fandom is selected.");
        result.AddError("    2) Fic size is >60k words.");
        result.AddError("Generic word searches will only be possible after DB is upgraded");
    }
    bool listNotSelected = ui->cbRecGroup->currentText().trimmed().isEmpty();
    if(listNotSelected && ui->cbSortMode->currentText() == "Rec Count")
    {
        result.AddError("Sorting on recommendation count only makes sense, ");
        result.AddError("if a recommendation list is selected");
        result.AddError("");
        result.AddError("Please select a recommendation list to the right of Rec List: or create one");
    }
    auto currentListId = env.interfaces.recs->GetCurrentRecommendationList();
    core::RecPtr list = env.interfaces.recs->GetList(currentListId);
    bool emptyList = !list || list->ficCount == 0;
    if(emptyList && (ui->cbSortMode->currentText() == "Rec Count"
                     || ui->chkSearchWithinList->isChecked()
                     || ui->sbMinimumListMatches->value() > 0))
    {
        result.AddError("Current filter doesn't make sense because selected recommendation list is empty");
    }
    return result;
}

core::StoryFilter MainWindow::ProcessGUIIntoStoryFilter(core::StoryFilter::EFilterMode mode,
                                                        bool useAuthorLink,
                                                        QString listToUse,
                                                        bool performFilterValidation)
{

    auto valueIfChecked = [](QCheckBox* box, auto value){
        if(box->isChecked())
            return value;
        return decltype(value)();
    };
    core::StoryFilter filter;
    auto filterState = ValidateFilter();
    if(performFilterValidation && filterState.hasErrors)
    {
        filter.isValid = false;
        QMessageBox::warning(nullptr, "Attention!",filterState.errors.join("\n"));
        return filter;
    }
    auto tags = ui->wdgTagsPlaceholder->GetSelectedTags();
    if(ui->chkEnableTagsFilter->isChecked())
        filter.activeTags = tags;
    qDebug() << "Active tags: " << filter.activeTags;
    filter.allowNoGenre = ui->chkNoGenre->isChecked();
    filter.allowUnfinished = ui->chkShowUnfinished->isChecked();
    filter.ensureActive = ui->chkActive->isChecked();
    filter.ensureCompleted= ui->chkComplete->isChecked();
    filter.fandom = GetCurrentFandomID();
    filter.otherFandomsMode = ui->chkOtherFandoms->isChecked();

    filter.genreExclusion = valueIfChecked(ui->chkGenreMinus, core::StoryFilter::ProcessDelimited(ui->leNotContainsGenre->text(), "###"));
    filter.genreInclusion = valueIfChecked(ui->chkGenrePlus,core::StoryFilter::ProcessDelimited(ui->leContainsGenre->text(), "###"));
    filter.wordExclusion = valueIfChecked(ui->chkWordsMinus, core::StoryFilter::ProcessDelimited(ui->leNotContainsWords->text(), "###"));
    filter.wordInclusion = valueIfChecked(ui->chkWordsPlus, core::StoryFilter::ProcessDelimited(ui->leContainsWords->text(), "###"));
    filter.ignoreAlreadyTagged = ui->chkIgnoreTags->isChecked();
    filter.crossoversOnly= ui->chkCrossovers->isChecked();
    filter.ignoreFandoms= ui->chkIgnoreFandoms->isChecked();
    filter.includeCrossovers =false; //ui->rbCrossovers->isChecked();
    //    filter.includeSlash = ui->chkOnlySlash->isChecked();
    //    filter.excludeSlash = ui->chkInvertedSlashFilter->isChecked();

    SlashFilterState slashState{
        ui->chkEnableSlashFilter->isChecked(),
                ui->chkApplyLocalSlashFilter->isChecked(),
                ui->chkInvertedSlashFilter->isChecked(),
                ui->chkOnlySlash->isChecked(),
                ui->chkInvertedSlashFilterLocal->isChecked(),
                ui->chkOnlySlashLocal->isChecked(),
                ui->chkEnableSlashExceptions->isChecked(),
                QList<int>{}, // temporary placeholder
        ui->cbSlashFilterAggressiveness->currentIndex()
    };

    filter.slashFilter = slashState;
    filter.maxFics = valueIfChecked(ui->chkRandomizeSelection, ui->sbMaxRandomFicCount->value());
    filter.minFavourites = valueIfChecked(ui->chkFaveLimitActivated, ui->sbMinimumFavourites->value());
    filter.maxWords= ui->cbMaxWordCount->currentText().toInt();
    filter.minWords= ui->cbMinWordCount->currentText().toInt();
    filter.randomizeResults = ui->chkRandomizeSelection->isChecked();
    filter.recentAndPopularFavRatio = ui->sbFavrateValue->value();
    filter.recentCutoff = ui->dteFavRateCut->dateTime();
    filter.reviewBias = static_cast<core::StoryFilter::EReviewBiasMode>(ui->cbBiasFavor->currentIndex());
    filter.biasOperator = static_cast<core::StoryFilter::EBiasOperator>(ui->cbBiasOperator->currentIndex());
    filter.reviewBiasRatio = ui->leBiasValue->text().toDouble();
    filter.sortMode = static_cast<core::StoryFilter::ESortMode>(ui->cbSortMode->currentIndex() + 1);
    filter.genreSortField = ui->leGenreSortField->text();
    filter.showOriginsInLists = ui->chkShowOrigins->isChecked();
    filter.minRecommendations =  ui->sbMinimumListMatches->value();
    filter.recordLimit = ui->chkLimitPageSize->isChecked() ?  ui->sbPageSize->value() : -1;
    filter.recordPage = ui->chkLimitPageSize->isChecked() ?  0 : -1;
    filter.listOpenMode = ui->chkSearchWithinList->isChecked();
    //if(ui->cbSortMode->currentText())
    if(listToUse.isEmpty())
        filter.listForRecommendations = env.interfaces.recs->GetListIdForName(ui->cbRecGroup->currentText());
    else
        filter.listForRecommendations = env.interfaces.recs->GetListIdForName(listToUse);
    //filter.titleInclusion = nothing for now
    filter.website = "ffn"; // just ffn for now
    filter.mode = mode;
    //filter.lastFetchedRecordID = env.currentLastFanficId;
    QString authorUrl = ui->leAuthorUrl->text();
    auto author = env.interfaces.authors->GetByWebID("ffn", url_utils::GetWebId(authorUrl, "ffn").toInt());
    if(author && useAuthorLink)
        filter.useThisRecommenderOnly = author->id;
    //filter.Log();
    return filter;
}



QSharedPointer<core::RecommendationList> MainWindow::BuildRecommendationParamsFromGUI()
{
    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->name = ui->cbRecListNames->currentText();
    params->tagToUse = ui->cbRecTagBuildGroup->currentText();
    params->minimumMatch = ui->sbMinRecMatch->value();
    params->pickRatio = ui->dsbMinRecThreshhold->value();
    params->alwaysPickAt = ui->sbAlwaysPickRecAt->value();
    return params;
}

void MainWindow::on_pbBuildRecs_clicked()
{
    TaskProgressGuard guard(this);
    BuildRecommendations(BuildRecommendationParamsFromGUI());
    //?  do I need to full reload here?
    env.interfaces.recs->LoadAvailableRecommendationLists();
}

void MainWindow::on_pbOpenAuthorUrl_clicked()
{
    QDesktopServices::openUrl(ui->leAuthorUrl->text());
}

void MainWindow::on_pbReprocessAuthors_clicked()
{
    TaskProgressGuard guard(this);
    env.ProcessListIntoRecommendations("lists/source.txt");
}

void MainWindow::on_cbRecTagBuildGroup_currentTextChanged(const QString &newText)
{
    auto list = env.interfaces.recs->GetList(newText);
    if(list)
    {
        ui->sbMinRecMatch->setValue(list->minimumMatch);
        ui->dsbMinRecThreshhold->setValue(list->pickRatio);
        ui->sbAlwaysPickRecAt->setValue(list->alwaysPickAt);
    }
    else
    {
        ui->sbMinRecMatch->setValue(1);
        ui->dsbMinRecThreshhold->setValue(150);
        ui->sbAlwaysPickRecAt->setValue(1);
        ui->cbRecListNames->setCurrentText("lupine");
    }
}

void MainWindow::OnCopyFavUrls()
{
    TaskProgressGuard guard(this);
    QClipboard *clipboard = QApplication::clipboard();
    QString result;
    for(int i = 0; i < recommendersModel->rowCount(); i ++)
    {
        auto author = env.interfaces.authors->GetAuthorByNameAndWebsite(recommendersModel->index(i, 0).data().toString(), "ffn");
        if(!author)
            continue;
        result += author->url("ffn") + "\n";
    }
    clipboard->setText(result);
}

void MainWindow::on_cbRecGroup_currentIndexChanged(const QString &arg1)
{
    env.interfaces.recs->SetCurrentRecommendationList(env.interfaces.recs->GetListIdForName(ui->cbRecGroup->currentText()));
    if(ui->chkSyncListNameToView->isChecked())
        ui->leCurrentListName->setText(ui->cbRecGroup->currentText());
    FillRecommenderListView();
}

void MainWindow::on_pbCreateNewList_clicked()
{
    TaskProgressGuard guard(this);
    QSharedPointer<core::RecommendationList> params;
    auto listName = ui->leCurrentListName->text().trimmed();
    auto listId = env.interfaces.recs->GetListIdForName(listName);
    if(listId != -1)
    {
        QMessageBox::warning(nullptr, "Warning!", "Can't create a list with a name that already exists, choose another name");
        return;
    }
    params->name = listName;
    env.interfaces.recs->LoadListIntoDatabase(params);
    FillRecTagCombobox();

}

void MainWindow::on_pbRemoveList_clicked()
{
    auto listName = ui->leCurrentListName->text().trimmed();
    auto listId = env.interfaces.recs->GetListIdForName(listName);
    if(listId == -1)
        return;
    auto button = QMessageBox::question(nullptr, "Question", "Do you really want to delete the recommendation list?");
    if(button == QMessageBox::No)
        return;
    env.interfaces.recs->DeleteList(listId);
    FillRecTagCombobox();
}

void MainWindow::on_pbAddAuthorToList_clicked()
{
    TaskProgressGuard guard(this);
    QString url = ui->leAuthorUrl->text().trimmed();
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);
    if(url.isEmpty())
        return;
    if(!LoadAuthor(url))
        return;
    QString listName = ui->leCurrentListName->text().trimmed();
    if(listName.isEmpty())
    {
        QMessageBox::warning(nullptr, "Warning!", "You need at least some name to create a new list. Please enter the list name.");
    }

    RecommendationsProcessor recProcessor(db, env.interfaces.fanfics,
                                          env.interfaces.fandoms,
                                          env.interfaces.authors,
                                          env.interfaces.recs);

    if(!recProcessor.AddAuthorToRecommendationList(ui->leCurrentListName->text().trimmed(), url))
        return; //todo add warning


    auto lists = env.interfaces.recs->GetAllRecommendationListNames();
    ui->cbRecGroup->setModel(new QStringListModel(lists));
    if(ui->cbRecGroup->currentText()== ui->leCurrentListName->text().trimmed())
        FillRecommenderListView(true);
}

void MainWindow::on_pbRemoveAuthorFromList_clicked()
{
    QString url = ui->leAuthorUrl->text().trimmed();
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);
    if(url.isEmpty())
        return;

    RecommendationsProcessor recProcessor(db, env.interfaces.fanfics,
                                          env.interfaces.fandoms,
                                          env.interfaces.authors,
                                          env.interfaces.recs);

    if(!recProcessor.RemoveAuthorFromRecommendationList(ui->leCurrentListName->text().trimmed(), url))
        return; //todo add warning

    if(ui->cbRecGroup->currentText()== ui->leCurrentListName->text().trimmed())
        FillRecommenderListView(true);
}

void MainWindow::OnCheckUnfinishedTasks()
{
    CheckUnfinishedTasks();
}

void MainWindow::on_chkRandomizeSelection_toggled(bool checked)
{
    //ui->chkRandomizeSelection->setEnabled(checked);
    ui->sbMaxRandomFicCount->setEnabled(checked);
}

void MainWindow::on_pbReinitFandoms_clicked()
{
    QString diagnostics;
    diagnostics+= "This operation will now reload fandom index pages.\n";
    diagnostics+= "It is only necessary if you need to add a new fandom.\n";
    diagnostics+= "Do you want to continue?\n";

    QMessageBox m;
    m.setIcon(QMessageBox::Warning);
    m.setText(diagnostics);
    auto yesButton =  m.addButton("Yes", QMessageBox::AcceptRole);
    auto noButton =  m.addButton("Cancel", QMessageBox::AcceptRole);
    Q_UNUSED(noButton);
    m.exec();
    if(m.clickedButton() != yesButton)
        return;

    UpdateFandomTask task;
    task.ffn = true;
    UpdateFandomList(task);
    env.interfaces.fandoms->Clear();
}

void MainWindow::OnGetAuthorFavouritesLinks()
{
    TaskProgressGuard guard(this);
    auto author = LoadAuthor(ui->leAuthorUrl->text());
    if(!author)
        return;
    auto list = env.interfaces.authors->GetAllAuthorsFavourites(author->id);
    QClipboard *clipboard = QApplication::clipboard();
    QString result;
    for(auto bit : list)
    {
        result += bit + "\n";
    }
    clipboard->setText(result);
    SetFinishedStatus();

}

void MainWindow::OnOpenLogUrl(const QUrl & url)
{
    QDesktopServices::openUrl(url);
}

void MainWindow::OnWipeCache()
{
    An<PageManager> pager;
    pager->WipeAllCache();
}


void MainWindow::UpdateFandomList(UpdateFandomTask )
{
    TaskProgressGuard guard(this);

    ui->edtResults->clear();

    QSqlDatabase db = QSqlDatabase::database();
    FandomListReloadProcessor proc(db, env.interfaces.fanfics, env.interfaces.fandoms, env.interfaces.pageTask, env.interfaces.db);
    connect(&proc, &FandomListReloadProcessor::displayWarning, this, &MainWindow::OnWarningRequested);
    connect(&proc, &FandomListReloadProcessor::requestProgressbar, this, &MainWindow::OnProgressBarRequested);
    connect(&proc, &FandomListReloadProcessor::updateCounter, this, &MainWindow::OnUpdatedProgressValue);
    connect(&proc, &FandomListReloadProcessor::updateInfo, this, &MainWindow::OnNewProgressString);
    proc.UpdateFandomList();
}


void MainWindow::on_chkCutoffLimit_toggled(bool checked)
{
    ui->deCutoffLimit->setEnabled(checked);
    if(checked)
        ui->chkIgnoreUpdateDate->setChecked(false);
}

void MainWindow::on_chkIgnoreUpdateDate_toggled(bool checked)
{
    if(checked)
        ui->chkCutoffLimit->setChecked(false);
}

void MainWindow::OnRemoveFandomFromRecentList()
{
    auto fandom = ui->lvTrackedFandoms->currentIndex().data(0).toString();
    env.interfaces.fandoms->RemoveFandomFromRecentList(fandom);
    env.interfaces.fandoms->ReloadRecentFandoms();
    recentFandomsModel->setStringList(env.interfaces.fandoms->GetRecentFandoms());
}

void MainWindow::OnRemoveFandomFromIgnoredList()
{
    auto fandom = ui->lvIgnoredFandoms->currentIndex().data(0).toString();
    env.interfaces.fandoms->RemoveFandomFromIgnoredList(fandom);
    ignoredFandomsModel->setStringList(env.interfaces.fandoms->GetIgnoredFandoms());
}

void MainWindow::OnRemoveFandomFromSlashFilterIgnoredList()
{
    auto fandom = ui->lvExcludedFandomsSlashFilter->currentIndex().data(0).toString();
    env.interfaces.fandoms->RemoveFandomFromIgnoredListSlashFilter(fandom);
    ignoredFandomsSlashFilterModel->setStringList(env.interfaces.fandoms->GetIgnoredFandomsSlashFilter());
}

void MainWindow::OnFandomsContextMenu(const QPoint &pos)
{
    fandomMenu.popup(ui->lvTrackedFandoms->mapToGlobal(pos));
}

void MainWindow::OnIgnoredFandomsContextMenu(const QPoint &pos)
{
    ignoreFandomMenu.popup(ui->lvIgnoredFandoms->mapToGlobal(pos));
}

void MainWindow::OnIgnoredFandomsSlashFilterContextMenu(const QPoint &pos)
{
    ignoreFandomSlashFilterMenu.popup(ui->lvExcludedFandomsSlashFilter->mapToGlobal(pos));
}

void MainWindow::on_pbFormattedList_clicked()
{
    if(ui->chkGroupFandoms->isChecked())
        OnDoFormattedListByFandoms();
    else
        OnDoFormattedList();
}

void MainWindow::on_pbCreateHTML_clicked()
{
    on_pbFormattedList_clicked();
    QString partialfileName = ui->cbRecGroup->currentText() + "_page_" + QString::number(env.pageOfCurrentQuery) + ".html";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    "FicLists/" + partialfileName,
                                                    tr("*.html"));
    if(fileName.isEmpty())
        return;

    QFile f(fileName);
    f.open( QIODevice::WriteOnly );
    QTextStream ts(&f);
    QClipboard *clipboard = QApplication::clipboard();
    QString header = "<!DOCTYPE html>"
                     "<html>"
                     "<head>"
                     "<title>Page Title</title>"
                     "</head>"
                     "<body>";
    QString footer = "</body>"
                     "</html>";
    ts << header;
    ts << clipboard->text();
    ts << footer;
    f.close();
}


void MainWindow::OnFindSimilarClicked(QVariant url)
{
    //auto id = url_utils::GetWebId(url.toString(), "ffn");
    auto id = env.interfaces.fanfics->GetIDFromWebID(url.toInt(),"ffn");
    if(id == -1)
        return;
    CreateSimilarListForGivenFic(id);
}

void MainWindow::on_pbIgnoreFandom_clicked()
{
    env.interfaces.fandoms->IgnoreFandom(ui->cbIgnoreFandomSelector->currentText(), ui->chkIgnoreIncludesCrossovers->isChecked());
    ignoredFandomsModel->setStringList(env.interfaces.fandoms->GetIgnoredFandoms());
}

void MainWindow::on_pbExcludeFandomFromSlashFiltering_clicked()
{
    env.interfaces.fandoms->IgnoreFandomSlashFilter(ui->cbIgnoreFandomSlashFilter->currentText());
    ignoredFandomsSlashFilterModel->setStringList(env.interfaces.fandoms->GetIgnoredFandomsSlashFilter());
}

void MainWindow::on_pbReloadAllAuthors_clicked()
{
    //ReprocessAllAuthors();
    //ReprocessAllAuthorsV2();
    ReprocessAllAuthorsJustSlash("slash_keywords");
}

void MainWindow::OnOpenAuthorListByID()
{
    int id = ui->leOpenID->text().toInt();
    auto author = env.interfaces.authors->GetById(id);
    if(!author)
        return;
    ui->leAuthorUrl->setText(author->url("ffn"));
    TimedAction action("Full open",[&](){
        on_pbOpenRecommendations_clicked();
    });
    action.run();
}

void MainWindow::on_pbCreateSlashList_clicked()
{
    auto authors = env.interfaces.authors->GetAllAuthors("ffn", true);
    QSqlDatabase db = QSqlDatabase::database();
    SlashProcessor slashProcessor(db,
                                  env.interfaces.fanfics,
                                  env.interfaces.fandoms,
                                  env.interfaces.pageTask,
                                  env.interfaces.authors,
                                  env.interfaces.recs,
                                  env.interfaces.db);
    slashProcessor.CreateListOfSlashCandidates(1, authors);
}

void MainWindow::on_pbProcessSlash_clicked()
{
    DetectSlashForEverythingV2();
}

void MainWindow::on_pbDoSlashFullCycle_clicked()
{
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);
    TimedAction filter("Full cycle", [&](){
        DoFullCycle();
    });
    filter.run();
    transaction.finalize();
}



void MainWindow::on_pbDisplayHumor_clicked()
{
    TaskProgressGuard guard(this);
    auto authors = env.interfaces.authors->GetAllAuthors("ffn", true);
    QSqlDatabase db = QSqlDatabase::database();
    HumorProcessor humorProcessor(db,
                                  env.interfaces.fanfics,
                                  env.interfaces.fandoms,
                                  env.interfaces.pageTask,
                                  env.interfaces.authors,
                                  env.interfaces.recs,
                                  env.interfaces.db);
    humorProcessor.CreateListOfHumorCandidates(authors);
}

void MainWindow::on_pbReloadAuthors_clicked()
{
    ReprocessAllAuthorsV2();
}

void MainWindow::OnPerformGenreAssignment()
{
    TaskProgressGuard guard(this);
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);
    env.interfaces.fanfics->PerformGenreAssignment();
    transaction.finalize();
}

void MainWindow::on_leOpenFicID_returnPressed()
{
    auto ficID = ui->leOpenFicID->text();
    auto fic = env.interfaces.fanfics->GetFicById(ficID.toInt());
    if(!fic)
        return;

    QDesktopServices::openUrl(url_utils::GetStoryUrlFromWebId(fic->ffn_id, "ffn"));
}

//void MainWindow::OnExportStatistics()
//{
//    auto closeDb = QSqlDatabase::database("StatisticsExport");
//    QSqlDatabase db = QSqlDatabase::database();
//    if(closeDb.isOpen())
//        closeDb.close();
//    QString exportFileName = "StatisticsDB";
//    bool success = QFile::remove(exportFileName + ".sql");
//    QSharedPointer<database::IDBWrapper> statisticsExportInterface (new database::SqliteInterface());
//    auto tagExportDb = statisticsExportInterface->InitDatabase(exportFileName, false);
//    statisticsExportInterface->ReadDbFile("dbcode/" + exportFileName + ".sql", exportFileName);
//    database::puresql::ExportTagsToDatabase(db, tagExportDb);
//}

void MainWindow::on_pbComedy_clicked()
{
    TaskProgressGuard guard(this);
    auto authors = env.interfaces.authors->GetAllAuthors("ffn", true);
    QSqlDatabase db = QSqlDatabase::database();
    HumorProcessor humorProcessor(db,
                                  env.interfaces.fanfics,
                                  env.interfaces.fandoms,
                                  env.interfaces.pageTask,
                                  env.interfaces.authors,
                                  env.interfaces.recs,
                                  env.interfaces.db);
    humorProcessor.CreateRecListOfHumorProfiles(authors);
}

void MainWindow::OnUpdatedProgressValue(int value)
{
    pbMain->setValue(value);
}

void MainWindow::OnNewProgressString(QString value)
{
    AddToProgressLog(value);
}

void MainWindow::OnResetTextEditor()
{
    ui->edtResults->clear();
}

void MainWindow::OnProgressBarRequested(int value)
{
    pbMain->setMaximum(value);
    pbMain->show();
}

void MainWindow::OnWarningRequested(QString value)
{
    QMessageBox::information(nullptr, "Attention!", value);
}

void MainWindow::OnFillDBIdsForTags()
{
    env.FillDBIDsForTags();
}

void MainWindow::OnTagReloadRequested()
{
    ProcessTagsIntoGui();
    tagList = env.interfaces.tags->ReadUserTags();
    qwFics->rootContext()->setContextProperty("tagModel", tagList);
}


void MainWindow::on_chkRecsAutomaticSettings_toggled(bool checked)
{
    if(checked)
    {
        ui->leRecsAlwaysPickAt->setEnabled(false);
        ui->leRecsPickRatio->setEnabled(false);
        ui->leRecsMinimumMatches->setEnabled(false);
        LoadAutomaticSettingsForRecListSources(ui->edtRecsContents->toPlainText().split("\n").size());
    }
    else
    {
        ui->leRecsAlwaysPickAt->setEnabled(true);
        ui->leRecsPickRatio->setEnabled(true);
        ui->leRecsMinimumMatches->setEnabled(true);
    }
}

void MainWindow::on_pbRecsLoadFFNProfileIntoSource_clicked()
{
    auto fics = LoadFavourteLinksFromFFNProfile(ui->leRecsFFNUrl->text());
    if(fics.size() == 0)
        return;

    LoadAutomaticSettingsForRecListSources(fics.size());
    ui->edtRecsContents->setOpenExternalLinks(false);
    ui->edtRecsContents->setOpenLinks(false);
    ui->edtRecsContents->setReadOnly(false);
    auto font = ui->edtRecsContents->font();
    font.setPixelSize(14);
    ui->edtRecsContents->setFont(font);

    for(auto fic: fics)
    {
        QString url = url_utils::GetStoryUrlFromWebId(fic->ffn_id, "ffn");
        QString toInsert = "<a href=\"" + url + "\"> %1 </a>";
        ui->edtRecsContents->insertHtml(fic->author->name + "<br>" +  fic->title + "<br>" + toInsert.arg(url) + "<br>");
        ui->edtRecsContents->insertHtml("<br>");
    }
}



void MainWindow::on_pbRecsCreateListFromSources_clicked()
{
    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->name = ui->leRecsListName->text();
    if(params->name.trimmed().isEmpty())
    {
        QMessageBox::warning(nullptr, "Warning!", "Please name your list.");
        return;
    }
    params->minimumMatch = ui->leRecsMinimumMatches->text().toInt();
    params->pickRatio = ui->leRecsPickRatio->text().toInt();
    params->alwaysPickAt = ui->leRecsAlwaysPickAt->text().toInt();
    TaskProgressGuard guard(this);
    QVector<int> sourceFics;
    QStringList lines = ui->edtRecsContents->toPlainText().split("\n");
    QRegularExpression rx("https://www.fanfiction.net/s/(\\d+)");
    for(auto line : lines)
    {
        if(!line.startsWith("http"))
            continue;
        auto match = rx.match(line);
        if(!match.hasMatch())
            continue;
        QString val = match.captured(1);
        sourceFics.push_back(val.toInt());
    }
    auto result = env.BuildRecommendations(params, sourceFics, false);
    if(result == -1)
    {
        QMessageBox::warning(nullptr, "Attention!", "Could not create a list with such parameters\n"
                                                    "Try using more source fics, or loosen the restrictions");
        return;
    }
    Q_UNUSED(result);
    auto lists = env.interfaces.recs->GetAllRecommendationListNames(true);
    SilentCall(ui->cbRecGroup)->setModel(new QStringListModel(lists));

    ui->cbSortMode->setCurrentText("Rec Count");
    on_pbLoadDatabase_clicked();
}





void MainWindow::on_pbReapplyFilteringMode_clicked()
{
    on_cbCurrentFilteringMode_currentTextChanged(ui->cbCurrentFilteringMode->currentText());
}

void MainWindow::ResetFilterUItoDefaults()
{
    ui->chkRandomizeSelection->setChecked(false);
    ui->chkEnableSlashFilter->setChecked(true);
    ui->chkEnableTagsFilter->setChecked(false);
    ui->chkIgnoreFandoms->setChecked(true);
    ui->chkComplete->setChecked(false);
    ui->chkShowUnfinished->setChecked(true);
    ui->chkActive->setChecked(false);
    ui->chkNoGenre->setChecked(true);
    ui->chkIgnoreTags->setChecked(false);
    ui->chkOtherFandoms->setChecked(false);
    ui->chkGenrePlus->setChecked(false);
    ui->chkGenreMinus->setChecked(false);
    ui->chkWordsPlus->setChecked(false);
    ui->chkWordsMinus->setChecked(false);
    ui->chkFaveLimitActivated->setChecked(false);
    ui->chkLimitPageSize->setChecked(true);
    ui->chkHeartProfile->setChecked(false);
    ui->chkInvertedSlashFilter->setChecked(true);
    ui->chkInvertedSlashFilterLocal->setChecked(true);
    ui->chkOnlySlash->setChecked(false);
    ui->chkOnlySlashLocal->setChecked(false);
    ui->chkApplyLocalSlashFilter->setChecked(true);
    ui->chkCrossovers->setChecked(false);
    ui->chkNonCrossovers->setChecked(false);

    ui->cbMinWordCount->setCurrentText("0");
    ui->cbMaxWordCount->setCurrentText("");
    ui->cbBiasFavor->setCurrentText("None");
    ui->cbBiasOperator->setCurrentText(">");
    ui->cbSlashFilterAggressiveness->setCurrentText("Light");
    ui->cbSortMode->setCurrentIndex(0);

    ui->leBiasValue->setText("11.");
    QDateTime dt = QDateTime::currentDateTimeUtc().addYears(-1);
    ui->dteFavRateCut->setDateTime(dt);
    ui->sbFavrateValue->setValue(4);
    ui->sbPageSize->setValue(100);
    ui->sbMaxRandomFicCount->setValue(6);
    ui->chkSearchWithinList->setChecked(false);
    ui->sbMinimumListMatches->setValue(0);

}

void MainWindow::on_cbCurrentFilteringMode_currentTextChanged(const QString &)
{
    if(ui->cbCurrentFilteringMode->currentText() == "Default")
    {
        ResetFilterUItoDefaults();
    }

    if(ui->cbCurrentFilteringMode->currentText() == "Random Recs")
    {
        ResetFilterUItoDefaults();
        ui->chkRandomizeSelection->setChecked(true);
        ui->sbMaxRandomFicCount->setValue(6);
        ui->cbSortMode->setCurrentText("Rec Count");
        ui->chkSearchWithinList->setChecked(true);
        ui->sbMinimumListMatches->setValue(1);
    }
    if(ui->cbCurrentFilteringMode->currentText() == "Tag Search")
    {
        ResetFilterUItoDefaults();
        ui->chkEnableTagsFilter->setChecked(true);
    }
}

void MainWindow::on_cbRecGroup_currentTextChanged(const QString &)
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.setValue("Settings/currentList", ui->cbRecGroup->currentText());
    settings.sync();
}

void MainWindow::on_pbUseProfile_clicked()
{
    on_pbRecsLoadFFNProfileIntoSource_clicked();
    ResetFilterUItoDefaults();
    on_pbRecsCreateListFromSources_clicked();
}

void MainWindow::on_pbMore_clicked()
{
    if(!ui->wdgDetailedRecControl->isVisible())
    {
        ui->spRecsFan->setCollapsible(0,0);
        ui->spRecsFan->setCollapsible(1,0);
        ui->wdgDetailedRecControl->show();
        ui->spRecsFan->setSizes({1000,0});
        ui->pbMore->setText("Less");
    }
    else
    {
        ui->spRecsFan->setCollapsible(0,0);
        ui->spRecsFan->setCollapsible(1,0);
        ui->wdgDetailedRecControl->hide();
        ui->spRecsFan->setSizes({0,1000});
        ui->pbMore->setText("More");
    }
}


void MainWindow::on_sbMinimumListMatches_valueChanged(int value)
{
    if(value > 0)
        ui->chkSearchWithinList->setChecked(true);
}

void MainWindow::on_chkOtherFandoms_toggled(bool checked)
{
    if(checked)
        ui->chkIgnoreFandoms->setChecked(false);
}

void MainWindow::on_chkIgnoreFandoms_toggled(bool checked)
{
    if(checked)
        ui->chkOtherFandoms->setChecked(false);
}
