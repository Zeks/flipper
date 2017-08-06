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
#include "libs/UniversalModels/include/TableDataInterface.h"
#include "libs/UniversalModels/include/TableDataListHolder.h"
#include "libs/UniversalModels/include/AdaptingTableModel.h"
#include "qml_ficmodel.h"
#include "include/section.h"
#include "include/pagegetter.h"

class QSortFilterProxyModel;
class QQuickWidget;
class QQuickView;
class QStringListModel;

namespace Ui {
class MainWindow;
}




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
    //void timerEvent(QTimerEvent *) override;

    bool CheckSectionAvailability();

private:

    void SetupFanficTable();
    void SetupTableAccess();
    //QTableView* types_table = nullptr;
    FicModel* typetableModel = nullptr;
    QSharedPointer<TableDataInterface> typetableInterface;
    TableDataListHolder<Section>* holder = nullptr;
    QList<Section> fanfics;
    QSortFilterProxyModel* sortModel;
    int processedFics = 0;
    ELastFilterButtonPressed currentSearchButton = ELastFilterButtonPressed::lfbp_search;
    int currentRecWave = 0;
    bool event(QEvent * e);
    void ReadSettings();
    void WriteSettings();

    void RequestAndProcessPage(QString fandom, QDateTime lastFandomUpdatedate, QString url, bool useLastIndex = false);
    WebPage RequestPage(QString, bool autoSaveToDB = false);


    QStringList GetCurrentFilterUrls(QString selectedFandom, bool crossoverState, bool ignoreTrackingState = false);
    QString GetFandom(QString text);

    void DisableAllLoadButtons();
    void EnableAllLoadButtons();

    //QString CreateURL(QString);

    void LoadData();
    QSqlQuery BuildQuery();
    QString BuildBias();

    //void LoadRecommendations(QString url);
    void LoadIntoDB(Section&);

    QString WrapTag(QString tag);
    void HideCurrentID();

    void UpdateFandomList(std::function<QString(Fandom)> linkGetter);
    void InsertFandomData(QMap<QPair<QString,QString>, Fandom> names);
    void PopulateComboboxes();

    QStringList GetCrossoverListFromDB();

    QStringList GetCrossoverUrl(QString, bool ignoreTrackingState = false);
    QStringList GetNormalUrl(QString, bool ignoreTrackingState = false);

    void OpenTagWidget(QPoint, QString url);
    void ReadTags();

    void SetTag(int id, QString tag);
    void UnsetTag(int id, QString tag);

    void ToggleTag();
    void CallExpandedWidget();

    QStringList SortedList(QStringList);
    QStringList ReverseSortedList(QStringList list);
    QStringList GetAllUniqueAuthorsFromRecommenders();
    void CreatePageThreadWorker();
    void StartPageWorker();
    void StopPageWorker();

    void ReinitProgressbar(int maxValue);
    void ShutdownProgressbar();
    void AddToProgressLog(QString);

    Ui::MainWindow *ui;
    int processedCount = 0;
    QString nextUrl;
    int timerId = -1;
    int pageCounter = 0;
    QMenu browserMenu;
    int currentRecommenderId = -1;
    QEventLoop managerEventLoop;
    QMap<QPair<QString,QString>, Fandom> names;
    QString currentProcessedSection;
    QString dbName = "CrawlerDB.sqlite";
    QDateTime lastUpdated;
    QHash<QString, Fandom> sections;
    QSignalMapper* mapper = nullptr;
    QProgressBar* pbMain = nullptr;
    QLabel* lblCurrentOperation = nullptr;
    QStringList currentFilterUrls;
    QString currentFilterUrl;
    bool ignoreUpdateDate = false;
    QStringList tagList;
    QStringListModel* tagModel;
    QStringListModel* recentFandomsModel;
    QStringListModel* recommendersModel;
    QHash<QString, Recommender> recommenders;
    QLineEdit* currentExpandedEdit = nullptr;
    TagWidget* tagWidgetDynamic = new TagWidget;
    QQuickWidget* qwFics = nullptr;
    QString currentFandom;
    bool isCrossover = false;
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

signals:
    void pageTask(QString, QString, QDateTime, bool);
    void pageTaskList(QStringList, bool);
};

