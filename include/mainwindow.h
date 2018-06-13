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
#pragma once

#include <QMainWindow>
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QMenu>
#include <QHash>
#include <QEventLoop>
#include <QSignalMapper>
#include <QSqlQuery>
#include <QProgressBar>
#include <QLabel>
#include <QTextBrowser>
#include <QTableView>
#include <QQueue>
#include <QThread>
#include <functional>
#include "tagwidget.h"
#include "storyfilter.h"
#include "libs/UniversalModels/include/TableDataInterface.h"
#include "libs/UniversalModels/include/TableDataListHolder.h"
#include "libs/UniversalModels/include/AdaptingTableModel.h"
#include "qml_ficmodel.h"
#include "include/core/section.h"
#include "include/pagetask.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/pagegetter.h"
#include "querybuilder.h"
#include "include/environment.h"

#include <QMovie>

class QSortFilterProxyModel;
class QQuickWidget;
class QQuickView;
class QStringListModel;
class ActionProgress;
class FFNFandomIndexParserBase;

namespace database {
class IDBWrapper;
}
namespace Ui {
class MainWindow;
}


struct BuildRecommendationParams{
    QString listName;
    int minTagCountMatch;
    int alwaysPickAuthorOnThisMatchCount;
    double threshholdRatio;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // current search mode
    // defines which search elemnts are to be used
    // and how
    enum ELastFilterButtonPressed
    {
        lfbp_search = 0,
        lfbp_recs = 1
    };
    explicit MainWindow(QWidget *parent = nullptr);

    //initalizes widgets
    void Init();


    ~MainWindow();

    // used to set up connections between database and interfaces
    // and between differnt interfaces themselves
    void InitInterfaces();
    // used to set up signal/slot connections
    void InitConnections();
    // sets up the timer that later triggers a check if there are unfinished tasks
    // written into the task databse
    void StartTaskTimer();
    // used to set up the approriate UI elemnts when task execution starts
    void InitUIFromTask(PageTaskPtr);

    // used to indicate "action in progress" status to the user
    void SetWorkingStatus();
    // used to indicate "action finished" status to the user
    void SetFinishedStatus();
    // used to indicate failure of the performed action to the user
    void SetFailureStatus();


    CoreEnvironment env;

private:

    // sets up the model for fanfics
    void SetupFanficTable();
    // sets up the interface to access fic params from core::fic class
    void SetupTableAccess();

    //bool event(QEvent * e);
    // reads settings into a files
    void ReadSettings();
    // writes settings into a files
    void WriteSettings();

    // a unified function to get the name of the current fandom
    // (without mentioning GUI element each time)
    QString GetCurrentFandomName();

    // a wrapper over pagegetter to request pages for fandom parsing
    //bool RequestAndProcessPage(QString fandom, QDate lastFandomUpdatedate, QString url);

    FandomParseTaskResult ProcessFandomSubTask(FandomParseTask);

    // a wrapper over pagegetter to request pages
    // todo probably could be dropped, will need to check
    WebPage RequestPage(QString,  ECacheMode forcedCacheMode = ECacheMode::use_cache, bool autoSaveToDB = false);


    // disable the interface (to perform a lengthy task)
    void DisableAllLoadButtons();

    // enables the interface (after some task has finished)
    void EnableAllLoadButtons();

    // use to actually query the search results from the database into fanfic list
    // placing results into the inetrface happens in PlaceResults
    void LoadData();

    void PlaceResults(); // places the currently filtered results into table

    // used to query the information of total query size from database
    // to separate stuff into pages
    int GetResultCount();



    // used to fill fandom database with new data from web
    void UpdateFandomList(UpdateFandomTask);

    // used to fill tagwidget
    void ProcessTagsIntoGui();

    // used to set tag to a certain fic
    void SetTag(int id, QString tag, bool silent = false);

    // used to unset tag from a certain fic
    void UnsetTag(int id, QString tag);

    // used to call a widget that llows editing of lineedit contents in a larger box
    void CallExpandedWidget();

    // creates page worker that gets pages from ffn in a thread
    void CreatePageThreadWorker();
    // starts page worker thread execution
    void StartPageWorker();
    // stops page worker thread execution
    void StopPageWorker();

    // shows the progress bar and set its max value to maxValue
    void ReinitProgressbar(int maxValue);

    // hides the progress bar and sets its value to 0
    void ShutdownProgressbar();

    // simply adds a line to QTextEdit with the results
    void AddToProgressLog(QString);

    // fills the sort by list combobox with names of recommendation lists
    void FillRecTagCombobox();

    // fills the authors list view with names from currently active recommendation list
    void FillRecommenderListView(bool forceRefresh = false);

    // loads the author favourites page into the database
    // used in recommendations page
    core::AuthorPtr LoadAuthor(QString url);

    // fill a recommendation list token to be passed into recommendation builder from ui
    QSharedPointer<core::RecommendationList> BuildRecommendationParamsFromGUI();



    PageTaskPtr ProcessFandomsAsTask(QList<core::FandomPtr> fandom,
                              QString taskComment,
                              bool allowCacheRefresh);

    // used to download the next wave of author favourites
    void LoadMoreAuthors();

    // a utility to pass a functor to all the authors
    void UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater);





    // creates a recommendation list from passed params
    int BuildRecommendations(QSharedPointer<core::RecommendationList> params, bool clearAuthors = true);

    // collects information from the ui into a token to be passed to a query generator
    core::StoryFilter ProcessGUIIntoStoryFilter(core::StoryFilter::EFilterMode, bool useAuthorLink = false, QString listToUse = QString());




    // pushes fandom to top of recent and reinits the recent fandom listview
    void ReinitRecent(QString name);

    // used to check for unfinished tasks on application start
    void CheckUnfinishedTasks();

    bool AskYesNoQuestion(QString);
    bool WarnCutoffLimit();
    bool WarnFullParse();

    void CrawlFandom(QString fandom);

    ECacheMode GetCurrentCacheMode() const;

    void CreateSimilarListForGivenFic(int);

    void ReprocessAllAuthorsV2();
    void ReprocessAllAuthorsJustSlash(QString fieldUsed);
    void DetectSlashForEverythingV2();

    void CreateListOfHumorCandidates(QList<core::AuthorPtr> authors);
    void CreateRecListOfHumorProfiles(QList<core::AuthorPtr> authors);

    void DoFullCycle();
    void UseAuthorTask(PageTaskPtr);
    ForcedFandomUpdateDate CreateForcedUpdateDateFromGUI();

//    QHash<int, int> CreateListOfNotSlashFics();
//    QHash<int, int> MatchSlashToNotSlash();

    Ui::MainWindow *ui;

    ELastFilterButtonPressed currentSearchButton = ELastFilterButtonPressed::lfbp_search;

    bool cancelCurrentTaskPressed = false;

    QStringList tagList; // user tags used in the system

    QTimer taskTimer; // used to initiate the warnign about unfinished tasks after the app window is shown

    std::function<void(int)> callProgress; // temporary shit while I decouple page getter from ui
    std::function<void(void)> cleanupEditor; // temporary shit while I decouple page getter from ui
    std::function<void(QString)> callProgressText; // temporary shit while I decouple page getter from ui

    QMovie refreshSpin; // an indicator that some work is in progress
    // using the Movie because it can animate while stuff is happening otherwise without much hassle from my side

    QSharedPointer<TableDataInterface> typetableInterface;
    FicModel* typetableModel = nullptr; // model for fanfics to be passed into qml
    TableDataListHolder<core::Fic>* holder = nullptr; // an interface class that model uses to access the data

    QStringListModel* recentFandomsModel= nullptr; // used in the listview that shows the recently search fandoms
    QStringListModel* ignoredFandomsModel= nullptr;
    QStringListModel* ignoredFandomsSlashFilterModel= nullptr;
    QStringListModel* recommendersModel = nullptr; // this keeps names of te authors in current recommendation list

    QLineEdit* currentExpandedEdit = nullptr; // expanded editor for line edits
    QDialog* expanderWidget = nullptr; // a dialog to display data from currentExpandedEdit for editing
    QTextEdit* edtExpander = new QTextEdit; //text edit taht contains the data of currentExpandedEdit while tis displayed by expanderWidget

    TagWidget* tagWidgetDynamic = new TagWidget; // tag filtering widget on Tags panel


    QQuickWidget* qwFics = nullptr; // a widget that holds qml fic search results
    QProgressBar* pbMain = nullptr; // a link to the progresspar on the ActionProgress widget
    QLabel* lblCurrentOperation = nullptr; // basically an expander so that actionProgress is shown to the right
    ActionProgress* actionProgress = nullptr;
    QMenu fandomMenu;
    QMenu ignoreFandomMenu;
    QMenu ignoreFandomSlashFilterMenu;


public slots:
    //broken and needs refactoring anyway
    //void ProcessFandoms(WebPage webPage);
    //void ProcessCrossovers(WebPage webPage);
    //void on_pbInit_clicked();
    // if anything, this needs to be rewritten to use fandom_ids
    // unsure if I want to give users that
    //void WipeSelectedFandom(bool);


    // triggered when user changes chapter in qml
    void OnChapterUpdated(QVariant, QVariant);
    // triggered when user adds tag in qml
    void OnTagAdd(QVariant tag, QVariant row);
    // triggered when user removes tag from a fic in qml
    void OnTagRemove(QVariant tag, QVariant row);

    // when new page arrives from page worker
    void OnNewPage(PageResult result);
    // places into the clipboard, the url of the fic clicked in qml
    void OnCopyFicUrl(QString);
    // used to open author pages on heart click that contain the current fic in their favourites
    void OnOpenRecommenderLinks(QString);
    // copies urls of all currently visible fics into the cliboard
    void OnCopyAllUrls();
    // used to create targeted HTML lists to put on the web
    void OnDoFormattedListByFandoms();
    void OnDoFormattedList();
    // queries and displays next page for the current query
    void OnDisplayNextPage();
    // queries and displays pervious page for the current query
    void OnDisplayPreviousPage();
    // queries and displays exact page for the current query
    void OnDisplayExactPage(int);

    // invoked on "Search" click
    void on_pbLoadDatabase_clicked();
private slots:
    // called to trudge through a fandom
    void on_pbCrawl_clicked();

    // used to receive tag events from tag widget on Tags tab
    void OnTagToggled(int, QString, bool);

    // used to make sure that the amount of random fics requested later will be correct
    void on_chkRandomizeSelection_clicked(bool checked);
    // used to toggle visibility of recommendation list selector for sorting
    void on_cbSortMode_currentTextChanged(const QString &arg1);

    // each of those functions calls an expanded editor for the corresponding line edit
    // -------------------------------------
    void on_pbExpandPlusGenre_clicked();
    void on_pbExpandMinusGenre_clicked();
    void on_pbExpandPlusWords_clicked();
    void on_pbExpandMinusWords_clicked();
    // -------------------------------------

    // used to put the clicked fandom into the fandom lieedit
    // and set the tracked tick for the fandom to the value from the database
    void OnNewSelectionInRecentList(const QModelIndex &current, const QModelIndex &previous);

    // used to put the clicked author name into the author combobox
    // and his url into the url lineedit
    void OnNewSelectionInRecommenderList(const QModelIndex &current, const QModelIndex &previous);

    // used to toggle the tracked status for fandom on the checkbox state change
    void on_chkTrackedFandom_toggled(bool checked);
    // used to downlaod the updates to all the tracked fandoms
    void on_pbLoadTrackedFandoms_clicked();
    // used to load author favourites for the url in the favourites lineedit
    void on_pbLoadPage_clicked();
    // used to open the recommendation for the author that is already in the databse
    // (and is selected in the author list)
    void on_pbOpenRecommendations_clicked();
    // used to full reload all authors in the current list
    void on_pbLoadAllRecommenders_clicked();
    // used to open favourites of all teh authors in the current list
    void on_pbOpenWholeList_clicked();
    void OpenRecommendationList(QString);
    // used to load the next wave of authors from LinkedAuthors
    void on_pbFirstWave_clicked();




    // used to create a recommendation list from GUI parameters (obscure, currently better use sources.txt)
    void on_pbBuildRecs_clicked();

    // used to open the url of the author currently selected in the favourites tab in the browser
    void on_pbOpenAuthorUrl_clicked();
    // probbaly legacy. Used to change the params in GUI rec list builder
    // since tags are no longer used should be phased out
    void on_cbRecTagBuildGroup_currentTextChanged(const QString &arg1);

    // misnamed. currently used to build recommedation lists from sources.txt
    void on_pbReprocessAuthors_clicked();

    // used to put urls for all authors in the current list into the clipboard
    void OnCopyFavUrls();

    // used to repopulate the interface elements dealing with recommendation lists
    // when the current selectionc hanges in the reclist combobox
    void on_cbRecGroup_currentIndexChanged(const QString &arg1);

    // used to manually create a new list to be populated
    void on_pbCreateNewList_clicked();
    // used to remove recommendation list from DB
    void on_pbRemoveList_clicked();
    // used to add the current author from the url in favourites tab to the current reclist
    void on_pbAddAuthorToList_clicked();
    // used to remove the current author in favourites tab from the current reclist
    void on_pbRemoveAuthorFromList_clicked();
    // triggered on timer to check if there is still an unfinished task
    void OnCheckUnfinishedTasks();
    // used to toggle the UI elements dealing with fic randomization
    void on_chkRandomizeSelection_toggled(bool checked);

    // used to trigger the reinit of fandoms on ffn
    void on_pbReinitFandoms_clicked();

    // used to get all favourites links for the current author on favourites page
    void OnGetAuthorFavouritesLinks();

    // used to toggle enabled status of date cutoff mechanism for fandom parses
    void on_chkCutoffLimit_toggled(bool checked);

    void on_chkIgnoreUpdateDate_toggled(bool checked);

    void OnRemoveFandomFromRecentList();
    void OnRemoveFandomFromIgnoredList();
    void OnRemoveFandomFromSlashFilterIgnoredList();


    void OnFandomsContextMenu(const QPoint &pos);
    void OnIgnoredFandomsContextMenu(const QPoint &pos);
    void OnIgnoredFandomsSlashFilterContextMenu(const QPoint &pos);

    void UpdateCategory(QString cat,
                        FFNFandomIndexParserBase* parser,
                        QSharedPointer<interfaces::Fandoms> fandomInterface);
    void OnOpenLogUrl(const QUrl&);

    void OnWipeCache();

    void on_pbFormattedList_clicked();
    void OnFindSimilarClicked(QVariant);

    void on_pbIgnoreFandom_clicked();

    void on_pbReloadAllAuthors_clicked();
    void OnOpenAuthorListByID();

    void on_pbCreateSlashList_clicked();

    void on_pbProcessSlash_clicked();

    void on_pbDoSlashFullCycle_clicked();

    //void on_pbOneMoreCycle_clicked();

    void on_pbExcludeFandomFromSlashFiltering_clicked();

    void on_pbDisplayHumor_clicked();

    void on_pbReloadAuthors_clicked();

    void OnPerformGenreAssignment();
    void on_leOpenFicID_returnPressed();

    void OnExportStatistics();

    void on_pbComedy_clicked();

    void OnUpdatedProgressValue(int);
    void OnNewProgressString(QString);
    void OnResetTextEditor();
    void OnProgressBarRequested(int);

signals:


    void pageTask(FandomParseTask);
    // page task when trudging through fandoms
    // contains the first url and the last url to stop
    //void pageTask(QString, QString, QDate, ECacheMode, bool);
    // page task when iterating through favourites pages
    // contains urls from a SUBtask
    void pageTaskList(QStringList, ECacheMode);
};

