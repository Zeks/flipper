/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "sql_abstractions/sql_query.h"
#include <QProgressBar>
#include <QLabel>
#include <QTextBrowser>
#include <QTableView>
#include <QQueue>
#include <QThread>
#include <functional>
#include "tagwidget.h"
#include "storyfilter.h"
#include "libs/ui-models/include/TableDataInterface.h"
#include "libs/ui-models/include/TableDataListHolder.h"
#include "libs/ui-models/include/AdaptingTableModel.h"
#include "qml_ficmodel.h"
#include "include/core/section.h"
#include "include/pagetask.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/pagegetter.h"
#include "querybuilder.h"
#include "include/environment.h"
#include <vector>

#include <QMovie>



class QSortFilterProxyModel;
class QQuickWidget;
class QQuickView;
class QStringListModel;
class ActionProgress;
class FFNFandomIndexParserBase;
class QRImageProvider;


namespace Ui {
class MainWindow;
}


struct BuildRecommendationParams{
    QString listName;
    int minTagCountMatch;
    int alwaysPickAuthorOnThisMatchCount;
    double threshholdRatio;
};

struct FilterErrors{
    void AddError(QString error){
        errors += error;
        hasErrors = true;
    }
    bool hasErrors = false;
    QStringList errors;
};



class ReclistCreationUIHelper
{
public:
    enum ESourcesMode{
        sm_profile = 0,
        sm_urls = 1,
        sm_tags = 2,
    };
    bool simpleMode = true;
    ESourcesMode sourcesMode = sm_profile;

    void SetupVisibilityForElements();


   QWidget* profileInput = nullptr;
   QWidget* advancedSettings = nullptr;
   QWidget* urlOuter= nullptr;
   QWidget* urlInner = nullptr;
   QWidget* main = nullptr;
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
    bool Init(bool scheduleSlashOn = false);


    ~MainWindow();
    void InitSortingCombobox();
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
    void DisplayInitialFicSelection();
    void DisplayRandomFicsForCurrentFilter();
    void QueueDefaultRecommendations();

    QSharedPointer<CoreEnvironment> env;

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
    QString GetCrossoverFandomName();

    int GetCurrentFandomID();
    int GetCrossoverFandomID();


    // a wrapper over pagegetter to request pages for fandom parsing
    //bool RequestAndProcessPage(QString fandom, QDate lastFandomUpdatedate, QString url);

    FandomParseTaskResult ProcessFandomSubTask(FandomParseTask);


    void SaveCurrentQuery();
    // use to actually query the search results from the database into fanfic list
    // placing results into the inetrface happens in PlaceResults
    void LoadData();

    void PlaceResults(); // places the currently filtered results into table

    // used to query the information of total query size from database
    // to separate stuff into pages
    int GetResultCount();



//    // used to fill fandom database with new data from web
//    void UpdateFandomList(UpdateFandomTask);

    // used to fill tagwidget
    void ProcessTagsIntoGui();

    // used to set tag to a certain fic
    void SetTag(int id, QString tag, bool silent = false);

    // used to unset tag from a certain fic
    void UnsetTag(int id, QString tag);

    // used to call a widget that llows editing of lineedit contents in a larger box
    void CallExpandedWidget();

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

    // a utility to pass a functor to all the authors
    void UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater);


    // creates a recommendation list from passed params
    int BuildRecommendations(QSharedPointer<core::RecommendationList> params, bool clearAuthors = true);

    // collects information from the ui into a token to be passed to a query generator
    core::StoryFilter ProcessGUIIntoStoryFilter(core::StoryFilter::EFilterMode,
                                                bool useAuthorLink = false,
                                                QString listToUse = QString(),
                                                bool performFilterValidation = true);
    void ProcessStoryFilterIntoGUI(core::StoryFilter filter);

    FilterErrors ValidateFilter();




    // pushes fandom to top of recent and reinits the recent fandom listview
    void ReinitRecent(QString name);


    bool AskYesNoQuestion(QString);

    ECacheMode GetCurrentCacheMode() const;

    void CreateSimilarListForGivenFic(int);

    void SetClientMode();
    void ResetFilterUItoDefaults(bool resetTagged = true);
    void DetectGenreSearchState();
    void DetectSlashSearchState();
    void LoadFFNProfileIntoTextBrowser(QTextBrowser *, QLineEdit *urlEdit, QLabel *infoLabel = nullptr);
    QVector<int> PickFicIDsFromTextBrowser(QTextBrowser*);
    QVector<int> PickFicIDsFromString(QString);
    void AnalyzeIdList(QVector<int>);
    void AnalyzeCurrentFilter();

    bool CreateRecommendationList(QSharedPointer<core::RecommendationList> params,
                                  QVector<int> sources);

    bool CreateDiagnosticRecommendationList(QSharedPointer<core::RecommendationList> params,
                                  QVector<int> sources);

    QSharedPointer<core::RecommendationList> CreateReclistParamsFromUI(bool silent = false);

    void LoadFrameIntoUI(const FilterFrame& frame);
    void SetPreviousEnabled(bool value);
    void SetNextEnabled(bool value);
    void FetchScoresForFics();
    bool CreateRecommendationListForCurrentMode();
    void PrepareUIToDisplayNewRecommendationList(QString name);

    struct FicSourceResult{
        QVector<int> sources;
        QString error;
    };

    FicSourceResult PickSourcesForEnteredProfile(QLineEdit* edit);
    FicSourceResult PickSourcesFromEditor();
    FicSourceResult PickSourcesFromTags();


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
    TableDataListHolder<QVector, core::Fanfic>* holder = nullptr; // an interface class that model uses to access the data

    QStringListModel* recentFandomsModel= nullptr; // used in the listview that shows the recently search fandoms
    QStringListModel* ignoredFandomsModel= nullptr;
    QStringListModel* ignoredFandomsSlashFilterModel= nullptr;
    QStringListModel* recommendersModel = nullptr; // this keeps names of te authors in current recommendation list

    QLineEdit* currentExpandedEdit = nullptr; // expanded editor for line edits
    QDialog* expanderWidget = nullptr; // a dialog to display data from currentExpandedEdit for editing
    QTextEdit* edtExpander = new QTextEdit; //text edit taht contains the data of currentExpandedEdit while tis displayed by expanderWidget

    //TagWidget* tagWidgetDynamic = new TagWidget; // tag filtering widget on Tags panel


    QQuickWidget* qwFics = nullptr; // a widget that holds qml fic search results
    QProgressBar* pbMain = nullptr; // a link to the progresspar on the ActionProgress widget
    QLabel* lblClientVersion = nullptr; // a copyable user id
    QLabel* lblUserIdStatic = nullptr; // a copyable user id
    QLabel* lblUserIdActive = nullptr; // a copyable user id
    QLabel* lblDBUpdateInfo = nullptr; // a copyable user id
    QLabel* lblDBUpdateDate = nullptr; // a copyable user id
    QLabel* lblCurrentOperation = nullptr; // basically an expander so that actionProgress is shown to the right
    ActionProgress* actionProgress = nullptr;
    QMenu fandomMenu;
    QMenu ignoreFandomMenu;
    QMenu ignoreFandomSlashFilterMenu;
    QRImageProvider* imgProvider = nullptr;
    QString primedTag;
    bool defaultRecommendationsQueued = false;
    ReclistCreationUIHelper reclistUIHelper;
    QString lastCreatedListName;
    QString styleSheetForAccept;
    QString styleSheetForReclistMenu;
    QString styleSheetForReclistCreation;
    bool reclistCreationShown = false;
    QString reclistToReturn;


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

    void OnHeartDoubleClicked(QVariant);
    void OnScoreAdjusted(QVariant, QVariant, QVariant);
    void OnSnoozeTypeChanged(QVariant, QVariant, QVariant);
    void OnSnoozeAdded(QVariant);
    void OnSnoozeRemoved(QVariant);
    void OnNotesEdited(QVariant, QVariant);

    void OnNewQRSource(QVariant);


    // triggered when user adds tag in qml
    void OnTagAddInTagWidget(QVariant tag, QVariant row);
    // triggered when user removes tag from a fic in qml
    void OnTagRemoveInTagWidget(QVariant tag, QVariant row);

    // places into the clipboard, the url of the fic clicked in qml
    void OnCopyFicUrl(QString);
    // copies urls of all currently visible fics into the cliboard
    void OnCopyAllUrls();
    // used to create targeted HTML lists to put on the web
    void OnDoFormattedListByFandoms();
    void OnDoFormattedList();
    // queries and displays next page for the current query
    void OnDisplayNextPage();

    void OnShuffleDisplayedData();

    // queries and displays pervious page for the current query
    void OnDisplayPreviousPage();
    // queries and displays exact page for the current query
    void OnDisplayExactPage(int);

    // invoked on "Search" click
    void on_pbLoadDatabase_clicked();
    void LoadAutomaticSettingsForRecListSources(int size);
    QSet<QString> LoadFavourteIdsFromFFNProfile(QString, QLabel *infoLabel = nullptr);
    void OnQMLRefilter();
    void OnQMLFandomToggled(QVariant);
    void OnQMLAuthorToggled(QVariant, QVariant active);
    void OnGetUrlsForTags(bool);
private slots:

    // used to receive tag events from tag widget on Tags tab
    void OnTagToggled(QString, bool);

    // used to make sure that the amount of random fics requested later will be correct
    void on_chkRandomizeSelection_clicked(bool checked);


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

    void OpenRecommendationList(QString);

    // used to put urls for all authors in the current list into the clipboard
    void OnCopyFavUrls();


    // used to toggle the UI elements dealing with fic randomization
    void on_chkRandomizeSelection_toggled(bool checked);

    void OnRemoveFandomFromRecentList();
    //void OnRemoveFandomFromIgnoredList();
    void OnRemoveFandomFromSlashFilterIgnoredList();


    void OnFandomsContextMenu(const QPoint &pos);
    //void OnIgnoredFandomsContextMenu(const QPoint &pos);
    void OnIgnoredFandomsSlashFilterContextMenu(const QPoint &pos);

//    void UpdateCategory(QString cat,
//                        FFNFandomIndexParserBase* parser,
//                        QSharedPointer<interfaces::Fandoms> fandomInterface);
    void OnOpenLogUrl(const QUrl&);

    void OnWipeCache();

    void GenerateFormattedList();
    void OnFindSimilarClicked(QVariant);

    //void on_pbIgnoreFandom_clicked();



    void OnUpdatedProgressValue(int);
    void OnNewProgressString(QString);
    void OnResetTextEditor();
    void OnProgressBarRequested(int);
    void OnWarningRequested(QString value);
    void OnFillDBIdsForTags();
    void OnTagReloadRequested();
    void OnClearLikedAuthorsRequested();

    void on_chkRecsAutomaticSettings_toggled(bool checked);

    void on_pbRecsLoadFFNProfileIntoSource_clicked();

    void on_pbRecsCreateListFromSources_clicked();


    void on_pbReapplyFilteringMode_clicked();

    void on_cbCurrentFilteringMode_currentTextChanged(const QString &arg1);

    void on_cbRecGroup_currentTextChanged(const QString &arg1);

    void on_pbCreateHTML_clicked();


    void on_sbMinimumListMatches_valueChanged(int arg1);

    void on_chkOtherFandoms_toggled(bool checked);

    //void on_chkIgnoreFandoms_toggled(bool checked);

    void on_pbDeleteRecList_clicked();

    void on_pbGetSourceLinks_clicked();


    void on_pbProfileCompare_clicked();

    void on_chkGenreUseImplied_stateChanged(int arg1);

    void on_cbSlashFilterAggressiveness_currentIndexChanged(int index);

    void on_pbLoadUrlForAnalysis_clicked();

    void on_pbAnalyzeListOfFics_clicked();

    void on_pbRefreshRecList_clicked();



    void on_chkCrossovers_stateChanged(int arg1);

    void on_pbFandomSwitch_clicked();

    void on_chkNonCrossovers_stateChanged(int arg1);

    void on_chkInvertedSlashFilter_stateChanged(int arg1);

    void on_chkOnlySlash_stateChanged(int arg1);

    void on_leAuthorID_returnPressed();

    void on_pbPreviousResults_clicked();

    void on_pbNextResults_clicked();

    void on_pbDiagnosticList_clicked();

    void on_cbRecGroupSecond_currentIndexChanged(const QString &arg1);

    void on_chkDisplayAuthorName_stateChanged(int arg1);

    void on_chkDisplaySecondList_stateChanged(int arg1);

    void on_chkDisplayComma_stateChanged(int arg1);


    void on_cbFicIDDisplayMode_currentIndexChanged(const QString &arg1);

    void on_chkDisplayDetectedGenre_stateChanged(int arg1);

    void on_pbVerifyUserFFNId_clicked();

    void on_cbStartupLoadSelection_currentIndexChanged(const QString &arg1);

    void on_leUserFFNId_editingFinished();

    void on_chkStopPatreon_stateChanged(int arg1);

    void on_rbSimpleMode_clicked();

    void on_rbAdvancedMode_clicked();

    void on_rbProfileMode_clicked();

    void on_rbUrlMode_clicked();

    void on_rbSelectedTagsMode_clicked();

    void on_pbNewRecommendationList_clicked();

    void on_leFFNProfileInputForUrls_returnPressed();


    void on_chkUseAwaysPickAt_stateChanged(int arg1);


    void on_pbValidateUserID_clicked();

    void onCopyDbUIDToClipboard(const QString&);
    void on_pbExpandIEntityds_clicked();

    void on_chkLikedAuthors_stateChanged(int arg1);

    void on_pbResetFilter_clicked();

    void OnTagFromClipboard();

signals:


    void pageTask(FandomParseTask);
    // page task when trudging through fandoms
    // contains the first url and the last url to stop
    //void pageTask(QString, QString, QDate, ECacheMode, bool);
    // page task when iterating through favourites pages
    // contains urls from a SUBtask
    void pageTaskList(QStringList, ECacheMode);
    void qrChange();
};

