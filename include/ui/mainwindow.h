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
#include "include/web/pagetask.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/web/pagegetter.h"
#include "querybuilder.h"
#include "include/flipper_client_logic.h"
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

    void SetupVisibilityForRecsCreatorElements();


   QWidget* profileInput = nullptr;
   QWidget* advancedSettings = nullptr;
   QWidget* urlOuter= nullptr;
   QWidget* urlInner = nullptr;
   QWidget* main = nullptr;
};




class MainWindow : public QMainWindow
{
    Q_OBJECT
    struct InitializationProgress{
        bool fandomListsReady = false;
        bool tagWidgetInterfacesFilled = false;
        bool fandomlistWidgetInitialized = false;
        bool recsUiHelperFilled = false;

    };

public:
    // this is public to let external setup happen
    QSharedPointer<FlipperClientLogic> env;


    explicit MainWindow(QWidget *parent = nullptr);

    //initalizes widgets
    bool InitFromReadyEnvironment(bool scheduleSlashOn = false);


    ~MainWindow();

    // used to set up signal/slot connections
    void InitConnections();
    // sets up the timer that later triggers a check if there are unfinished tasks
    // written into the task databse
    void StartTaskTimer();

    // used to indicate "action in progress" status to the user
    void SetWorkingStatus();
    // used to indicate "action finished" status to the user
    void SetFinishedStatus();
    // used to indicate failure of the performed action to the user
    void SetFailureStatus();

    void DisplayInitialFicSelection();
    void DisplayRandomFicsForCurrentFilter();
    void QueueDefaultRecommendations();



private:
    // used to actually query the new search results from the database into the internal env object
    // placing results into the inetrface happens in PlaceResults
    void FetchDataForFilters();
    // actually places the currently filtered results into table
    void PlaceResultsIntoTable();

    // used to query the information of total query size from database
    // to separate stuff into pages
    int GetResultCount();

    // the four functions below are syntactic candy
    // to access their params without knowing what eactly ui element is providing those
    // and without the need to manually do custom conversions each time
    QString GetCurrentFandomName();
    QString GetCrossoverFandomName();
    int GetCurrentFandomID();
    int GetCrossoverFandomID();

    // will store the current filtering state into a FilterFrame object and push it onto the history stack
    void SaveCurrentFilteringFrame();
    // will take a previously recorded filter frame and load all of its data into the ui
    void LoadFrameIntoUI(const FilterFrame& frame);
    // enables or disables the filter history navigation buttons
    void SetPreviousEnabled(bool value);
    void SetNextEnabled(bool value);


    /////////////////////////////////// INITIALIZATION BLOCK///////////////////////////////
    // init func for main combobox chaging the who app's fic sorting mode
    void InitFanficListSortingCombobox();

    // used to setup "clear" button for line edits that is not shown by  default
    // but necessary for a lot fo them in the application
    void EnableClearButtonForRelevantLineEdits();
    // window has quite a bit of ui elements that by deafult should be hidden
    // leftover and development stuff
    void PerformDefaultElementHide();
    // sets up custom colors and styles for various elements
    void InstallCustomPalettesAndStylesheets();
    // sets up the helping wrapper for recommendation list part with pointers
    // calls the default visibility setup function
    void InitializeRecCreationWidgetWithDefaultState();
    // some of the ui elements of the main window require tooltips to be displayed immediately
    // as opposed to after a couple seconds of hovering. this ensures that
    void EnableImmediateTooltipsForRelevantElements();
    // will check how much times the app has been run and colorize patreon tab if it is enough
    void PerformPatreonColorization();
    // will hide dev stuff from non dev builds
    // kind of stale - todo
    void PerformDevBuildUiAdjustment();
    // will disable the back/forward buttons initially
    void InitializeHistoryNavigationButtons();
    // new's the poiinters for models ised in this window
    void InstantiateModels();
    // initializes  the app's filters with defaults
    void InitializeDefaultSplitterSizes();
    // will acess user's id from env and try to parse a page from ffn to make sure the profile exists
    // will fail depending on the cloudflare's mood on any given day
    // failure i not critical for app's operations
    // it will just make ID red
    void VerifyUserFFNId();
    // sets up tag widget with pointers to classes it needs to acess the database
    void InitInterfacesForTagsWidget(QSharedPointer<FlipperClientLogic> env);
    // initializes pointers to classes it needs to acess the database
    // initializes the tree inside the widget
    void InitFandomListWidget(QSharedPointer<FlipperClientLogic> env);
    // will acess the database to fill various comboboxes with fandom list data
    void ReadFandomListsIntoUI(QSharedPointer<FlipperClientLogic> env);
    // creates status bar for the applications
    void CreateStatusBar(QSharedPointer<FlipperClientLogic> env);
    // sets up the generic table model to work for for fanfics
    void SetupFanficTable(QSharedPointer<FlipperClientLogic> env);
    // sets up the interface to access fic params from core::fic class
    // called from the func above
    void SetupAccessorsForGenericFanficTable();

    // fills up several menus with their actions
    void FillContextMenuActions();

    // reads properties for qml root context that are stored in settings
    // this needs to be called before the qml file is loaded in quickwidget
    void InitializeQmlPropertiesFromSettings(QQuickWidget* qwFics);
    // sets up connections between qml and c++ parts
    void InitializeQmlConnectionsToCpp(QQuickWidget* qwFics);

    // used to fill tagwidget with tags read from the user database
    void ReadTagsIntoUI(QSharedPointer<FlipperClientLogic> env);

    // fills the sort by list combobox with names of recommendation lists
    void ReadRecommendationListStateIntoUI(QSharedPointer<FlipperClientLogic> env);

    // reads settings into a files
    void ReadSettings();
    // writes settings into a files
    void WriteSettings();
    /////////////////////////////////// END INITIALIZATION BLOCK///////////////////////////////

    // used to set tag to a certain fic
    void SetTag(int ficId, QString tag, bool silent = false);

    // used to unset tag from a certain fic
    void UnsetTag(int ficId, QString tag);

    // used to call a widget that allows editing of lineedit contents in a larger box
    void CallExpandedWidget();

    // shows the progress bar and set its max value to maxValue
    void ReinitProgressbar(int maxValue);

    // hides the progress bar and sets its value to 0
    void ShutdownProgressbar();

    // simply adds a line to QTextEdit with the results
    void AddToProgressLog(QString);



    // fills the authors list view with names from currently active recommendation list
    // in reality only fills the model and nothing more
    // listview is not possible for  a public flipper build
    void FillRecommenderListView(bool forceRefresh = false);


    // collects information from the ui into a token to be passed to a query generator
    // @listToUse will force a select for a different list than what is currently selected
    // for service functions
    core::StoryFilter ProcessGUIIntoStoryFilter(core::StoryFilter::EFilterMode,
                                                QString listToUse = QString());
    // will read the storyFilter token into the required ui state all around
    void ProcessStoryFilterIntoGUI(core::StoryFilter filter);
    // will perform default initialization of every filtering ui element
    void ResetFilterUItoDefaults(bool resetTagged = true);

    // reads ui stateof the recommendations widget into an object storing the corresponding state
    QSharedPointer<core::RecommendationList> CreateReclistParamsFromUI(bool silent = false);

    // warns and disallows some currently impossible filter cases
    // that either make no sense or take too long
    FilterErrors ValidateFilter();

    // used to clean up the textedit currently used for displaying fic urls
    // whether it's the one in recommendations tab
    // or the one in fic list analysys tab
    void ResetUrlListEditor(QTextBrowser*edit);
    // fills the textedit currently used for displaying fic urls with provided urls
    void FillFicUrlsIntoRecsCreator(QTextBrowser *edit, const QSet<QString>&);

    // given the target @textbrowser
    // will read the usr profile id from @urlEdit
    // and fill the text browser with favourites data parsed from that profile
    // displays the resulting warnings in @infoLabel
    void LoadFFNProfileIntoTextBrowser(QTextBrowser * textbrowser, QLineEdit *urlEdit, QLabel *infoLabel = nullptr);


    // these two functions read either a string or textBrowser contents
    // and fill the resulting list with lla the ffn fic ids found within its contents
    QVector<int> PickFicIDsFromTextBrowser(QTextBrowser*);
    QVector<int> PickFicIDsFromString(QString);


    // pushes fandom to top of recent and reinits the recent fandom listview
    void PushAndReinitRecentFandoms(QString name);


    // currently unused, but realistically could be any time
    bool AskYesNoQuestion(QString);


    // given the fic id will create the simplest "recommendation list" for it
    // and display it in the main listview
    // todo, this currently pulls too much from env
    // suggests a named function in env itself
    void CreateSimilarListForGivenFic(int);


    // depending on whether implied genre search is enabled or not
    // enables or disables associated ui elements
    void InitializeUiElementsForCurrentGenreSearchState();
    // depending on the current slash filtering level
    // will enabled or disable several ui elements
    void InitializeUiElementsForCurrentSlashSearchState();


    // will fill the "analysis" tab of flipper with statistical data about the given fics set
    // todo this seems to have logic that is better living elsewhere
    void AnalyzeIdList(QVector<int>);
    // will fill the "analysis" tab of flipper with statistical data about the currently filtered fic set
    void AnalyzeCurrentFilter();

    // it's a thin wrapper over env->Buildrecommendation list with a warning
    bool CreateRecommendationList(QSharedPointer<core::RecommendationList> params,
                                  QVector<int> sources);





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



    Ui::MainWindow *ui;

    // this is used to make sure functions are not called in the wrong order
    // after they are "optimized"
    InitializationProgress initProgress;;

    //QStringList tagList; // user tags used in the system
    QTimer taskTimer; // used to initiate the warnign about unfinished tasks after the app window is shown



    QMovie refreshSpin; // an indicator that some work is in progress
    // using the Movie because it can animate while stuff is happening otherwise without much hassle from my side

    QSharedPointer<TableDataInterface> qmlFanficModelInterface;
    FicModel* qmlFanficModel = nullptr; // model for fanfics to be passed into qml
    TableDataListHolder<QVector, core::Fanfic>* backendFanficDataKeeper = nullptr; // an interface class that model uses to access the data

    QStringListModel* recentFandomsModel= nullptr; // used in the listview that shows the recently search fandoms
    QStringListModel* ignoredFandomsModel= nullptr;
    QStringListModel* ignoredFandomsSlashFilterModel= nullptr;
    QStringListModel* recommendersModel = nullptr; // this keeps names of te authors in current recommendation list

    QLineEdit* currentExpandedEdit = nullptr; // expanded editor for line edits
    QDialog* expanderWidget = nullptr; // a dialog to display data from currentExpandedEdit for editing
    QTextEdit* edtExpander = new QTextEdit; //text edit taht contains the data of currentExpandedEdit while tis displayed by expanderWidget

    ReclistCreationUIHelper reclistUIHelper;

    QQuickWidget* qwFics = nullptr; // a widget that holds qml fic search results
    QProgressBar* pbMain = nullptr; // a link to the progresspar on the ActionProgress widget
    ActionProgress* actionProgress = nullptr;

    QLabel* lblClientVersion = nullptr; // a copyable user id
    QLabel* lblUserIdStatic = nullptr; // a copyable user id
    QLabel* lblUserIdActive = nullptr; // a copyable user id
    QLabel* lblDBUpdateInfo = nullptr; // a copyable user id
    QLabel* lblDBUpdateDate = nullptr; // a copyable user id
    QLabel* lblCurrentOperation = nullptr; // basically an expander so that actionProgress is shown to the right

    QMenu fandomMenu;
    QMenu ignoreFandomMenu;
    QMenu ignoreFandomSlashFilterMenu;

    QRImageProvider* imgProvider = nullptr;

    QString lastCreatedListName;
    QString styleSheetForAccept;
    QString styleSheetForReclistMenu;
    QString styleSheetForReclistCreation;
    QString reclistToReturn;

    bool reclistCreationShown = false;
    bool defaultRecommendationsQueued = false;


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
    QSet<QString> LoadFavourteIdsFromFFNProfile(QString, QLabel *infoLabel = nullptr);
    bool VerifyProfileUrlString(QString);
    void OnQMLRefilter();
    void OnQMLFandomToggled(QVariant);
    void OnQMLAuthorToggled(QVariant, QVariant active);
    void OnGetUrlsForTags(bool);

    // custom filtering modes
    void SetRecommenderFilteringMode(QStringList);
    void SetAuthorFilteringMode(QStringList);
    void SetIdFilteringMode(QStringList);
    void UnsetAuthorFilteringMode();



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


    // used to put urls for all authors in the current list into the clipboard
    // this is highly intrusive and should never be a part of the public build
    // unless hidden by some obscure parameter
    void OnCopyRecommenderUrls();


    // used to toggle the UI elements dealing with fic randomization on and off
    void on_chkRandomizeSelection_toggled(bool checked);

    void OnRemoveFandomFromRecentList();
    void OnRemoveFandomFromSlashFilterIgnoredList();


    void OnFandomsContextMenu(const QPoint &pos);
    void OnIgnoredFandomsSlashFilterContextMenu(const QPoint &pos);

    void OnOpenUrl(const QUrl&);

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
    void qrChange();
};

