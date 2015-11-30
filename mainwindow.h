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

struct Section
{
    int start = 0;
    int end = 0;
    int wordCount = 0;
    int summaryEnd = 0;
    int wordCountStart = 0;
    QString fandom;
    QString title;
    QString genre;
    QString summary;
    QString author;
    QString url;
    QDateTime published;
    QDateTime updated;
    bool isValid =false;
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
    void RequestPage(QString);
    void ProcessPage(QString);
    QString GetFandom(QString text);
    Section GetSection( QString text, int start);
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetTitle(Section& , int& startfrom, QString text);
    void GetGenre(Section& , int& startfrom, QString text);
    void GetSummary(Section& , int& startfrom, QString text);
    void GetWordCount(Section& , int& startfrom, QString text);
    void GetPublishedDate(Section& , int& startfrom, QString text);
    void GetUpdatedDate(Section& , int& startfrom, QString text);
    void GetUrl(Section& , int& startfrom, QString text);
    void GetNext(Section& , int& startfrom, QString text);
    QDateTime ConvertToDate(QString);
    void SkipPages(int);
    QString CreateURL(QString);

    void LoadData();
    void HideFanfic(int id);
    void SmutFanfic(int id);
    void UnknownFandom(int id);
    void timerEvent(QTimerEvent *);


    void LoadIntoDB(Section&);
    QNetworkAccessManager manager;

private:
    Ui::MainWindow *ui;
    int processedCount = 0;
    QString nextUrl;
    int timerId = -1;
    int pageCounter = 0;
    QMenu browserMenu;

public slots:
    void OnNetworkReply(QNetworkReply*);
private slots:
    void on_pbCrawl_clicked();
    void OnHideFanfic();
    void OnSmutFanfic();
    void OnUnknownFandom();
    void OnShowContextMenu(QPoint);

    void on_pbLoadDatabase_clicked();
    void on_chkSmut_toggled(bool checked);
    void on_chkUnknownFandoms_toggled(bool checked);
};

#endif // MAINWINDOW_H
