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
    // will load favourite fic set from @profile into a set which is returned
    // while diagnostics are put into @infoLabel
    QSet<QString> LoadFavourteIdsFromFFNProfile(QString profile, QLabel *infoLabel = nullptr);
    // verifies that the provided url is a valid ffn url
    bool VerifyProfileUrlString(QString);

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

    // will fetch the scores breakdown for current ficset from LOCAL user database
    void FetchScoresForFics();

    // it's a thin wrapper over env->Buildrecommendation list with a warning
    bool CreateRecommendationList(QSharedPointer<core::RecommendationList> params,
                                  QVector<int> sources);

    // will crate a new recommendation list based on the state of recommendations cretor widget
    // i.e will read settings from currently active elements
    bool CreateRecommendationListForCurrentMode();

    // will set up the relevant default filtering options to display the newly creaated recommendation list
    void PrepareUIToDisplayNewRecommendationList(QString name);


    // helper struct to display errors in fetching sources for recommendation list
    // after soruce function returned
    struct FicSourceResult{
        QVector<int> sources;
        QString error;
    };
    // will read user profile and fill QVector<int> with favourited fic ids from it
    FicSourceResult PickSourcesForEnteredProfile(QLineEdit* edit);
    // will read text edit with urls and fill QVector<int> with fic ids from it
    FicSourceResult PickSourcesFromEditor();
    // will read the currently selected tags (in tags widget way below)
    // and fill QVector<int> with relevant fic ids for everything tagged with selected tags
    FicSourceResult PickSourcesFromTags();



    // sets up a mode that filters fics excusively in favourite lists from a list of recommenders in db
    void SetRecommenderFilteringMode(QStringList);
    // sets up a mode that filters fics excusively written by a provided author id
    void SetAuthorFilteringMode(QStringList);
    // sets up ui for id filtering mode, the exact mode is set externally (fic, author, recommenders)
    void SetIdFilteringMode(QStringList);
    // unsets the ui that triggers id filtering mode to happen
    void UnsetAuthorFilteringMode();


    // depending on a state of checkbox will call either generation of webpage with formatted list of selected fics
    // or a list of currently selecred fics grouped by fandom
    void GenerateFormattedList();
    // used to create targeted HTML lists to put on the web
    void GenerateFormattedListByFandoms();
    void GenerateDefaultFormattedList();

    ////////////////////// CLASS DATA BLOCK ////////////////////

    Ui::MainWindow *ui;

    // this is used to make sure functions are not called in the wrong order
    // after they are "optimized"
    InitializationProgress initProgress;;

    //QStringList tagList; // user tags used in the system
    QMovie refreshSpin; // an indicator that some work is in progress
    // using the Movie because it can animate while stuff is happening otherwise without much hassle from my side

    QSharedPointer<TableDataInterface> qmlFanficModelInterface;
    FicModel* qmlFanficModel = nullptr; // model for fanfics to be passed into qml
    TableDataListHolder<QVector, core::Fanfic>* backendFanficDataKeeper = nullptr; // an interface class that model uses to access the data

    QStringListModel* recentFandomsModel= nullptr; // used in the listview that shows the recently search fandoms
    // this is supposed to hold fandoms that should not be slash filtered
    // but it's too complex for the end user and is currently unused
    QStringListModel* ignoredFandomsSlashFilterModel= nullptr;
    QStringListModel* recommendersModel = nullptr; // this keeps names of te authors in current recommendation list

    QLineEdit* currentExpandedEdit = nullptr; // expanded editor for line edits
    QDialog* expanderWidget = nullptr; // a dialog to display data from currentExpandedEdit for editing
    QTextEdit* edtExpander = new QTextEdit; //text edit that contains the data of currentExpandedEdit while tis displayed by expanderWidget

    // helper instance to control the state and behaviour of recommendations widget
    ReclistCreationUIHelper reclistUIHelper;

    QQuickWidget* qwFics = nullptr; // a widget that holds qml view of fic search results
    QProgressBar* pbMain = nullptr; // a link to the progresspar on the ActionProgress widget
    ActionProgress* actionProgress = nullptr;

    QLabel* lblClientVersion = nullptr;
    QLabel* lblUserIdStatic = nullptr;
    QLabel* lblUserIdActive = nullptr; // a copyable user id
    QLabel* lblDBUpdateInfo = nullptr;
    QLabel* lblDBUpdateDate = nullptr;
    QLabel* lblCurrentOperation = nullptr; // basically an expander so that actionProgress is shown to the right

    QMenu menuRecentFandoms;
    QMenu menuFandomLIsts;
    QMenu menuSlashFilterIgnores;

    // image provider for qr code inside the qml view
    QRImageProvider* imgProvider = nullptr;

    QString lastCreatedListName;
    QString styleSheetForAccept;
    QString styleSheetForReclistMenu;
    QString styleSheetForReclistCreation;
    QString reclistToReturn;

    bool reclistCreationShown = false;
    bool defaultRecommendationsQueued = false;

    ////////////////////// END CLASS DATA BLOCK ////////////////////
public slots:
    // triggered when user changes chapter in qml
    void OnChapterUpdated(QVariant, QVariant);
    // triggered when user adds tag in qml
    void OnTagAdd(QVariant tag, QVariant row);
    // triggered when user removes tag from a fic in qml
    void OnTagRemove(QVariant tag, QVariant row);

    // calls a new fic set consisting only of fics present in lists of people
    // that recommended the fic that the heart was clicked on
    void OnHeartDoubleClicked(QVariant);
    // called wjhen user clicks on stars  to the right of the fic
    void OnScoreAdjusted(QVariant, QVariant, QVariant);
    // called when user fiddles with snooze type and range for fic in qml
    void OnSnoozeTypeChanged(QVariant, QVariant, QVariant);
    // called when user adds a snooze to the fic i nqms
    void OnSnoozeAdded(QVariant);
    // called when user removes a snooze in qm
    void OnSnoozeRemoved(QVariant);
    // called when user finishes editing of notes for a fic in qml
    void OnNotesEdited(QVariant, QVariant);

    // called to request a new qr code from image provider when user hovers
    // over respetive button on qml
    void OnNewQRSource(QVariant);

    // triggered when user adds tag in qml
    void OnTagAddInTagWidget(QVariant tag, QVariant row);
    // triggered when user removes tag from a fic in qml
    void OnTagRemoveInTagWidget(QVariant tag, QVariant row);

    // places into the clipboard, the url of the fic clicked in qml
    void OnCopyFicUrl(QString);
    // copies urls of all currently visible fics into the cliboard
    void OnCopyAllUrls();

    // queries and displays next page for the current query
    void OnDisplayNextPage();

    void OnShuffleDisplayedData();

    // queries and displays pervious page for the current query
    void OnDisplayPreviousPage();
    // queries and displays exact page for the current query
    void OnDisplayExactPage(int);

    // invoked on "Search" click
    void OnRefilterRequested();


    // catches refilter events from qml and refilters the table
    // current amount of such eventts - none
    void OnQMLRefilter();
    // catches clicks on fandoms in qml and passes them onto the ignore fandom combobox
    void OnQMLFandomToggled(QVariant);
    // catches the clicks on "show fics by author" in qml and refilters the table respectively
    void OnQMLAuthorToggled(QVariant, QVariant active);

    // used to read the tags in tagwidget filter and pull all of the fic urls
    // for the fics that are marked with selected tags
    // into the clipboard
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


    // used to put urls for all authors in the current list into the clipboard
    // this is highly intrusive and should never be a part of the public build
    // unless hidden by some obscure parameter
    void OnCopyRecommenderUrls();


    // used to toggle the UI elements dealing with fic randomization on and off
    void on_chkRandomizeSelection_toggled(bool checked);

    // triggered on context menu item to remove a fandom from recent fandom searches list
    void OnRemoveFandomFromRecentList();
    // triggered on context menu item to remove a fandom from slash filter ignore list
    void OnRemoveFandomFromSlashFilterIgnoredList();

    // currently not called todo
    void OnFandomsContextMenu(const QPoint &pos);
    // todo probably remove this as I don't think this is ever enabled
    void OnIgnoredFandomsSlashFilterContextMenu(const QPoint &pos);

    // called to open url in a browser
    void OnOpenUrl(const QUrl&);


    // called when user clicks on "pinpoint" in qml
    // causes a refilter into a "simple merged list of fics favu=ourited with this one"
    void OnFindSimilarClicked(QVariant);


    // these are all called on signals from backaend as it processes data
    void OnUpdatedProgressValue(int);
    void OnNewProgressString(QString);
    void OnResetTextEditor();
    void OnProgressBarRequested(int);
    void OnWarningRequested(QString value);


    // calls a service function that fills db ids for fics imported from TagExported file
    void OnFillDBIdsForTags();
    // called from import tag routine to reload after all of the tags have been read
    void OnTagReloadRequested();

    // called from tagwidget when user checks "show only fics from authors of the tagged fics"
    // to clear up the "only liked authors" check as these are exclusive
    void OnClearLikedAuthorsRequested();

    // this toggles several recommendation list ui elements on and off
    // depending on the state of "automatic" checkbox
    void on_chkRecsAutomaticSettings_toggled(bool checked);

    // will load favourites from a provided ffn profile into rec sources list
    void on_pbRecsLoadFFNProfileIntoSource_clicked();

    // will create actual recommendation list from prefilled filled sources editor
    void on_pbRecsCreateListFromSources_clicked();

    // this is disabled and hidden now
    void on_pbReapplyFilteringMode_clicked();

    // this is disabled and hidden now
    void on_cbCurrentFilteringMode_currentTextChanged(const QString &arg1);


    // refetches metascores for fics upon selection of a new rec list
    void on_cbRecGroup_currentTextChanged(const QString &arg1);

    // triggers on "html" button and creates a file with current fics in the filter
    // optionally grouped by fandoms
    void on_pbCreateHTML_clicked();

    // this auto,atically enables "search within list as long as checkbox next to metascore value filter is not null and enabled
    void on_sbMinimumListMatches_valueChanged(int arg1);

    // calls deletion of currently selected recommendation list
    void on_pbDeleteRecList_clicked();

    // copies to clipboard the sources for currently selected recommendation list
    void on_pbGetSourceLinks_clicked();


    // this would compare favourites between two profiles if that actually wasn't dangerous because of cloudflare limits
    void on_pbProfileCompare_clicked();

    // triggers using implied genre mode mode on and off depending on the ui state
    void on_chkGenreUseImplied_stateChanged(int arg1);

    // used to reinit the rest of the slash filter ui when the user changes the aggressivenewss level
    void on_cbSlashFilterAggressiveness_currentIndexChanged(int index);

    // loads up user profile for analysis (requires parsing so a no go atm)
    void on_pbLoadUrlForAnalysis_clicked();

    // analyzes a list of fic ids in the second tab of the app
    // works either with parsing first or just dumping a list directly
    void on_pbAnalyzeListOfFics_clicked();

    // this uses the stored information about the recommendation list
    // to fully recreate it with new fics from the server
    void on_pbRefreshRecList_clicked();

    // will show/hide the second fandom combobox based on the state of "crossovers" checkbox
    void on_chkCrossovers_stateChanged(int arg1);

    // switches the values of crossovers and normal fandom filter combobox
    void on_pbFandomSwitch_clicked();

    // will show/hide the second fandom combobox based on the state of "crossovers" checkbox
    void on_chkNonCrossovers_stateChanged(int arg1);

    // this ui is hidden and unnecessary
    void on_chkInvertedSlashFilter_stateChanged(int arg1);

    // this will untik a control that is currently not visible
    void on_chkOnlySlash_stateChanged(int arg1);

    // will enabled id search mode (will tick the checkbox) and refilter to show fics for author
    void on_leAuthorID_returnPressed();

    // search history navigation -> next
    void on_pbPreviousResults_clicked();
    // search history navigation -> previous
    void on_pbNextResults_clicked();

    // this is for service function only and is not facing the user
    // will load up some diagnostics about fics and lists into the dev database
    void on_pbDiagnosticList_clicked();

    // experimental stale feature of second recommendation list and comparison
    // not intended to work rn
    void on_cbRecGroupSecond_currentIndexChanged(const QString &arg1);

    // triggered from settings to show/hide author name in fic's sheet
    void on_chkDisplayAuthorName_stateChanged(int arg1);

    // stale feature
    void on_chkDisplaySecondList_stateChanged(int arg1);

    // used to enabled/disable comma between thousand in fic's wordcount
    void on_chkDisplayComma_stateChanged(int arg1);

    // changes the meanign of the number tot the left of the fic
    // can be position in current selection or position in the full reclist
    void on_cbFicIDDisplayMode_currentIndexChanged(const QString &arg1);

    // will enable/disable the display of detected genre instead of author set
    void on_chkDisplayDetectedGenre_stateChanged(int arg1);

    // will trigger a reparse of the user fav list to test if it's valid
    // depending on cloudfkare's mood it can fail
    void on_pbVerifyUserFFNId_clicked();

    // will change what exactly is loaded upon the app startup
    // - current top of reclist
    // - random selection
    // - nothing
    void on_cbStartupLoadSelection_currentIndexChanged(const QString &arg1);

    // calls on_pbVerifyUserFFNId_clicked when user edits their ffn id
    void on_leUserFFNId_editingFinished();

    // will stop coloring patreon tab is the user ticks the respective checkbox
    void on_chkStopPatreon_stateChanged(int arg1);

    // these all deal with differnt modes of creating recommednation list  in reclist widget
    void on_rbSimpleMode_clicked();
    void on_rbAdvancedMode_clicked();
    void on_rbProfileMode_clicked();
    void on_rbUrlMode_clicked();
    void on_rbSelectedTagsMode_clicked();
    // this will show/hide the recommendation list creation subwidget
    // when the suer clicks on "new recommendationm list"
    void on_pbNewRecommendationList_clicked();


    // will parse ffn profile for favourites once return has been pressed in the respective lineedit
    // in recs creation widget
    void on_leFFNProfileInputForUrls_returnPressed();

    // older controls, will change the state of "always pick at" if "automatic" is not checked
    void on_chkUseAwaysPickAt_stateChanged(int arg1);

    // will parse and validate user id
    void on_pbValidateUserID_clicked();

    // will copy the unique id of the user database to cliboard for error reporting
    void onCopyDbUIDToClipboard(const QString&);
    // calls expanded editor for "id" search lineedit
    void on_pbExpandIEntityds_clicked();

    // will clear the "authors for tags" checkbox in tags widget since it's exclusive with liked authors
    void on_chkLikedAuthors_stateChanged(int arg1);

    // will reset application filter to default state
    void on_pbResetFilter_clicked();

    // will read fic list inside the clipboard and assign currently selected tags to them
    // very service~ey, very experimental and  likely unsafe
    void OnTagFromClipboard();

signals:

    void qrChange();
};

