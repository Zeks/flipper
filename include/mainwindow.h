#ifndef MAINWINDOW_H

#define MAINWINDOW_H

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
#include <functional>
#include "tagwidget.h"
#include "libs/UniversalModels/include/TableDataInterface.h"
#include "libs/UniversalModels/include/TableDataListHolder.h"
#include "libs/UniversalModels/include/AdaptingTableModel.h"
#include "qml_ficmodel.h"
class QSortFilterProxyModel;
class QQuickWidget;
class QQuickView;
class QStringListModel;
struct Section
{
    int ID = -1;
    int start = 0;
    int end = 0;

    int summaryEnd = 0;
    int wordCountStart = 0;
    int statSectionStart=0;
    int statSectionEnd=0;
    int complete=0;
    int atChapter=0;

    QString wordCount = 0;
    QString chapters = 0;
    QString reviews = 0;
    QString favourites= 0;
    QString rated= 0;


    QString fandom;
    QString title;
    QString genre;
    QString summary;
    QString statSection;
    QString author;
    QString url;
    QString tags;
    QString origin;
    QString language;
    QDateTime published;
    QDateTime updated;
    QString characters;
    bool isValid =false;
};
struct Fandom
{
    QString name;
    QString section;
    QString url;
    QString crossoverUrl;
};
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void Init();
    void timerEvent(QTimerEvent *) override;

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

    bool event(QEvent * e);
    void ReadSettings();

    void RequestPage(QString);
    void ProcessPage(QString);

    QString GetFandom(QString text);
    Section GetSection( QString text, int start);
    QString GetCurrentFilterUrl();
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetTitle(Section& , int& startfrom, QString text);
    void GetGenre(Section& , int& startfrom, QString text);
    void GetSummary(Section& , int& startfrom, QString text);
    void GetCrossoverFandomList(Section& , int& startfrom, QString text);
    void GetWordCount(Section& , int& startfrom, QString text);
    void GetPublishedDate(Section& , int& startfrom, QString text);
    void GetUpdatedDate(Section& , int& startfrom, QString text);
    void GetUrl(Section& , int& startfrom, QString text);
    void GetNext(Section& , int& startfrom, QString text);
    void GetStatSection(Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    QDateTime GetMaxUpdateDateForSection(QStringList sections);

    QString CreateURL(QString);

    void LoadData();
    void LoadIntoDB(Section&);

    QString WrapTag(QString tag);
    void HideCurrentID();

    void UpdateFandomList(QNetworkAccessManager& manager,
                          std::function<QString(Fandom)> linkGetter);
    void InsertFandomData(QMap<QPair<QString,QString>, Fandom> names);
    void PopulateComboboxes();

    QStringList GetFandomListFromDB();
    QStringList GetCrossoverListFromDB();

    QString GetCrossoverUrl(QString);
    QString GetNormalUrl(QString);

    void OpenTagWidget(QPoint, QString url);
    void ReadTags();

    void SetTag(int id, QString tag);
    void UnsetTag(int id, QString tag);

    void ToggleTag();

    Ui::MainWindow *ui;
    int processedCount = 0;
    QString nextUrl;
    int timerId = -1;
    int pageCounter = 0;
    QMenu browserMenu;
    QEventLoop managerEventLoop;
    QMap<QPair<QString,QString>, Fandom> names;
    QString currentProcessedSection;
    QString dbName = "CrawlerDB.sqlite";
    QDateTime lastUpdated;
    QHash<QString, Fandom> sections;
//    QHash<QString, QString> nameOfFandomSectionToLink;
//    QHash<QString, QString> nameOfCrossoverSectionToLink;
    QSignalMapper* mapper = nullptr;
    QProgressBar* pbMain = nullptr;
    QLabel* lblCurrentOperation = nullptr;
    QNetworkAccessManager manager;
    QNetworkAccessManager fandomManager;
    QNetworkAccessManager crossoverManager;
    QString currentFilterUrl;
    bool ignoreUpdateDate = false;
    QStringList tagList;
    QStringListModel* tagModel;
    TagWidget* tagWidgetDynamic = new TagWidget;
    QQuickWidget* qwFics = nullptr;
    QString currentFandom;
    bool isCrossover = false;
    void PopulateIdList(std::function<QSqlQuery(QString)> bindQuery, QString query, bool forceUpdate = false);
    QString AddIdList(QString query, int count);
    QString CreateLimitQueryPart();
    QHash<QString, QList<int>> randomIdLists;


public slots:
    void OnNetworkReply(QNetworkReply*);
    void OnFandomReply(QNetworkReply*);
    void OnCrossoverReply(QNetworkReply*);
    void OnChapterUpdated(QVariant, QVariant, QVariant);
    void OnTagAdd(QVariant tag, QVariant row);
    void OnTagRemove(QVariant tag, QVariant row);
    void OnTagClicked(QVariant tag, QVariant currentMode, QVariant row);
    void WipeSelectedFandom(bool);
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


    void on_chkRandomizeSelection_clicked(bool checked);
};

#endif // MAINWINDOW_H
