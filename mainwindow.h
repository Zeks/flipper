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
    int complete=0;

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
    void timerEvent(QTimerEvent *) override;

    bool CheckSectionAvailability();
private:


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
    QDateTime GetMaxUpdateDateForSection(QString section);

    QString CreateURL(QString);

    void LoadData();
    void LoadIntoDB(Section&);

    QString WrapTag(QString tag);
    void HideCurrentID();

    void UpdateFandomList();
    void UpdateCrossoverList();
    void PopulateComboboxes();

    QStringList GetFandomListFromDB();
    QStringList GetCrossoverListFromDB();

    QString GetCrossoverUrl(QString);
    QString GetNormalUrl(QString);

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
    QSignalMapper* mapper = nullptr;
    QProgressBar* pbMain = nullptr;
    QLabel* lblCurrentOperation = nullptr;
    QNetworkAccessManager manager;
    QNetworkAccessManager fandomManager;
    QNetworkAccessManager crossoverManager;
    QString currentFilterurl;

public slots:
    void OnNetworkReply(QNetworkReply*);
    void OnFandomReply(QNetworkReply*);
    void OnCrossoverReply(QNetworkReply*);
private slots:
    void OnSetTag(QString);
    void OnShowContextMenu(QPoint);
    void OnSectionChanged(QString);
    void OnCheckboxFilter(int);
    void on_pbLoadDatabase_clicked();
    void on_pbInit_clicked();
    void on_pbCrawl_clicked();

};

#endif // MAINWINDOW_H
