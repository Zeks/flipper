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
#include <QProgressBar>
#include <QLabel>
#include <functional>



struct Section
{
    int start = 0;
    int end = 0;

    int summaryEnd = 0;
    int wordCountStart = 0;
    int statSectionStart=0;
    int statSectionEnd=0;

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
    QString url;
    QString section;
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
    void RequestPage(QString);
    void ProcessPage(QString);
    QString GetFandom(QString text);
    Section GetSection( QString text, int start);
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetTitle(Section& , int& startfrom, QString text);

    void GetStatSection(Section& , int& startfrom, QString text);

    void GetGenre(Section& , int& startfrom, QString text);
    void GetSummary(Section& , int& startfrom, QString text);
    void GetCrossoverFandomList(Section& , int& startfrom, QString text);
    void GetWordCount(Section& , int& startfrom, QString text);
    void GetPublishedDate(Section& , int& startfrom, QString text);
    void GetUpdatedDate(Section& , int& startfrom, QString text);
    void GetUrl(Section& , int& startfrom, QString text);
    void GetNext(Section& , int& startfrom, QString text);

    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    QDateTime ConvertToDate(QString);
    void SkipPages(int);
    QString CreateURL(QString);

    void LoadData();
    void HideFanfic(int id);
    void SmutFanfic(int id);
    void UnknownFandom(int id);
    void timerEvent(QTimerEvent *);


    void UpdateFandomList();
    void UpdateCrossoverList();
    void PopulateComboboxes();
    QStringList GetFandomListFromDB();
    QStringList GetCrossoverListFromDB();
    QString GetCrossoverUrl(QString);
    QString GetNormalUrl(QString);

    QDateTime GetMaxUpdateDateForSection(QString section);

    void LoadIntoDB(Section&);
    QNetworkAccessManager manager;
    QNetworkAccessManager fandomManager;
    QNetworkAccessManager crossoverManager;
    bool CheckSectionAvailability();
private:
    Ui::MainWindow *ui;
    int processedCount = 0;
    QString nextUrl;
    int timerId = -1;
    int pageCounter = 0;
    QMenu browserMenu;
    QEventLoop managerEventLoop;
    QHash<QString, QList<Fandom>> names;
    QString currentProcessedSetion;
    QString dbName = "CrawlerDB.sqlite";
    QDateTime lastUpdated;
    QHash<QString, QString> nameOfFandomSectionToLink;
    QHash<QString, QString> nameOfCrossoverSectionToLink;
    QSignalMapper* mapper;
    QProgressBar* pbMain;
    QLabel* lblCurrentOperation;
    QString WrapTag(QString tag);
    void HideCurrentID();
    QString GetCurrentFilterUrl();




public slots:
    void OnNetworkReply(QNetworkReply*);
    void OnFandomReply(QNetworkReply*);
    void OnCrossoverReply(QNetworkReply*);
private slots:
    void on_pbCrawl_clicked();
    void OnSetTag(QString);
//    void OnSmutFanfic();
    //void OnUnknownFandom();
    void OnShowContextMenu(QPoint);
    void OnSectionChanged(QString);
    void on_pbLoadDatabase_clicked();
    void on_chkSmut_toggled(bool checked);
    void on_chkUnknownFandoms_toggled(bool checked);
    void on_pbInit_clicked();
    void on_chkEverything_toggled(bool checked);
    void on_chkMeh_toggled(bool checked);
    void on_chkReadQueue_toggled(bool checked);
    void on_chkCrapFandom_toggled(bool checked);
    void on_chkReading_toggled(bool checked);
    void on_chkFinished_toggled(bool checked);
    void on_checkBox_toggled(bool checked);
    void on_chkDisgusting_toggled(bool checked);
};

#endif // MAINWINDOW_H
