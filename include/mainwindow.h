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
#include "include/section.h"
#include "include/pagegetter.h"
#include "querybuilder.h"

class QSortFilterProxyModel;
class QQuickWidget;
class QQuickView;
class QStringListModel;
namespace database{
class DBFandomsBase;
class DBFanficsBase;
class DBAuthorsBase;
class IDBWrapper;
class Tags;
class DBRecommendationListsBase;
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
    enum ELastFilterButtonPressed
    {
        lfbp_search = 0,
        lfbp_recs = 1
    };
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void Init();
    void InitConnections();
    bool CheckSectionAvailability();

private:

    void SetupFanficTable();
    void SetupTableAccess();
    FicModel* typetableModel = nullptr;
    QSharedPointer<TableDataInterface> typetableInterface;
    TableDataListHolder<core::Fic>* holder = nullptr;
    QList<core::Fic> fanfics;
    QSortFilterProxyModel* sortModel;
    int processedFics = 0;
    ELastFilterButtonPressed currentSearchButton = ELastFilterButtonPressed::lfbp_search;
    int currentRecWave = 0;
    bool event(QEvent * e);
    void ReadSettings();
    void WriteSettings();

    QString GetCurrentFandomName();

    void RequestAndProcessPage(QString fandom, QDateTime lastFandomUpdatedate, QString url);
    WebPage RequestPage(QString,  ECacheMode forcedCacheMode = ECacheMode::use_cache, bool autoSaveToDB = false);



    QString GetFandom(QString text);

    void DisableAllLoadButtons();
    void EnableAllLoadButtons();

    void LoadData();
    QSqlQuery BuildQuery();
    QString BuildBias();

    QString WrapTag(QString tag);
    void HideCurrentID();

    void UpdateFandomList(std::function<QString(core::Fandom)> linkGetter);
    void InsertFandomData(QMap<QPair<QString,QString>, core::Fandom> names);
    void PopulateComboboxes();

    QStringList GetCrossoverListFromDB();

    QStringList GetCrossoverUrl(QString);
    QStringList GetNormalUrl(QString);

    void OpenTagWidget(QPoint, QString url);
    void ProcessTagsIntoGui();

    void SetTag(int id, QString tag, bool silent = false);
    void UnsetTag(int id, QString tag);

    void ToggleTag();
    void CallExpandedWidget();

    QStringList SortedList(QStringList);
    QList<QSharedPointer<core::Author> > ReverseSortedList(QList<QSharedPointer<core::Author> > list);
    QStringList GetUniqueAuthorsFromActiveRecommenderSet();

    void CreatePageThreadWorker();
    void StartPageWorker();
    void StopPageWorker();

    void ReinitProgressbar(int maxValue);
    void ShutdownProgressbar();
    void AddToProgressLog(QString);
    void FillRecTagBuildCombobox();
    void FillRecTagCombobox();

    QSharedPointer<core::RecommendationList> BuildRecommendationParamsFromGUI();

    Ui::MainWindow *ui;
    int processedCount = 0;
    QString nextUrl;
    int timerId = -1;
    int pageCounter = 0;
    QMenu browserMenu;
    QEventLoop managerEventLoop;
    QMap<QPair<QString,QString>, core::Fandom> names;
    QString currentProcessedSection;
    QString dbName = "CrawlerDB.sqlite";
    QDateTime lastUpdated;
    QHash<QString, core::Fandom> sections;
    QSignalMapper* mapper = nullptr;
    QProgressBar* pbMain = nullptr;
    QLabel* lblCurrentOperation = nullptr;
    //QStringList currentFilterUrls;
    QString currentFilterUrl;
    bool ignoreUpdateDate = false;
    QStringList tagList;
    QStringListModel* tagModel;
    QStringListModel* recentFandomsModel= nullptr;
    QStringListModel* recommendersModel = nullptr;
    QLineEdit* currentExpandedEdit = nullptr;
    TagWidget* tagWidgetDynamic = new TagWidget;
    QQuickWidget* qwFics = nullptr;
    void PopulateIdList(std::function<QSqlQuery(QString)> bindQuery, QString query, bool forceUpdate = false);
    QString AddIdList(QString query, int count);
    QString CreateLimitQueryPart();
    QHash<QString, QList<int>> randomIdLists;
    QDialog* expanderWidget = nullptr;
    QTextEdit* edtExpander = new QTextEdit;
    QTimer selectionTimer;
    QThread pageThread;
    PageThreadWorker* worker = nullptr;
    QList<WebPage> pageQueue;
    core::DefaultQueryBuilder queryBuilder;
    core::StoryFilter filter;
    //QHash<QString, core::RecommendationList> lists;


    QSharedPointer<database::DBFandomsBase> fandomsInterface;
    QSharedPointer<database::DBFanficsBase> fanficsInterface;
    QSharedPointer<database::DBAuthorsBase> authorsInterface;
    QSharedPointer<database::Tags> tagsInterface;
    QSharedPointer<database::DBRecommendationListsBase> recsInterface;
    QSharedPointer<database::IDBWrapper> dbWrapperInterface;


    void LoadMoreAuthors(bool reprocessCache = false);
    void ReparseAllAuthors(bool reprocessCache = false);
    void ProcessTagIntoRecommenders(QString tag);
    void UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater);
    void ReprocessAuthors();
    void ProcessListIntoRecommendations(QString list);
    void BuildRecommendations(QSharedPointer<core::RecommendationList> params);
    core::StoryFilter ProcessGUIIntoStoryFilter(core::StoryFilter::EFilterMode);
public slots:
    void ProcessFandoms(WebPage webPage);
    void ProcessCrossovers(WebPage webPage);
    void OnChapterUpdated(QVariant, QVariant, QVariant);
    void OnTagAdd(QVariant tag, QVariant row);
    void OnTagRemove(QVariant tag, QVariant row);
    void OnTagClicked(QVariant tag, QVariant currentMode, QVariant row);
    void WipeSelectedFandom(bool);
    void OnNewPage(WebPage);
    void OnCopyFicUrl(QString);
    void OnCopyAllUrls();
private slots:
    void OnSetTag(QString);
    void OnShowContextMenu(QPoint);
    void OnSectionChanged(QString);
    void OnCheckboxFilter(int);
    void on_pbLoadDatabase_clicked();
    void on_pbInit_clicked();
    void on_pbCrawl_clicked();
    void OnLinkClicked(const QUrl &);

    void OnTagToggled(int, QString, bool);
    void OnCustomFilterClicked();
    void OnSectionReloadActivated();

    void on_chkRandomizeSelection_clicked(bool checked);
    void on_cbCustomFilters_currentTextChanged(const QString &arg1);
    void on_cbSortMode_currentTextChanged(const QString &arg1);
    void on_pbExpandPlusGenre_clicked();
    void on_pbExpandMinusGenre_clicked();
    void on_pbExpandPlusWords_clicked();
    void on_pbExpandMinusWords_clicked();
    void OnNewSelectionInRecentList(const QModelIndex &current, const QModelIndex &previous);
    void OnNewSelectionInRecommenderList(const QModelIndex &current, const QModelIndex &previous);

    void on_chkTrackedFandom_toggled(bool checked);
    void on_rbNormal_clicked();
    void on_rbCrossovers_clicked();
    void on_pbLoadTrackedFandoms_clicked();
    void on_pbLoadPage_clicked();
    void on_pbRemoveRecommender_clicked();
    void on_pbOpenRecommendations_clicked();
    void on_pbLoadAllRecommenders_clicked();
    void on_pbOpenWholeList_clicked();
    void on_pbFirstWave_clicked();
    void OnReloadRecLists();
    void on_cbUseDateCutoff_clicked();

    void on_pbBuildRecs_clicked();

    void on_cbRecTagGroup_currentIndexChanged(const QString &arg1);

    void on_pbOpenAuthorUrl_clicked();

    void on_pbReprocessAuthors_clicked();

    void on_cbRecTagBuildGroup_currentTextChanged(const QString &arg1);

signals:
    void pageTask(QString, QString, QDateTime, ECacheMode);
    void pageTaskList(QStringList, ECacheMode);
};

