#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

struct Section
{
    int start = 0;
    int end = 0;
    int wordCount = 0;
    int summaryEnd = 0;
    int wordCountStart = 0;
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
    Section GetSection( QString text, int start);
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetTitle(Section& , int& startfrom, QString text);
    void GetGenre(Section& , int& startfrom, QString text);
    void GetSummary(Section& , int& startfrom, QString text);
    void GetWordCount(Section& , int& startfrom, QString text);
    void GetPublishedDate(Section& , int& startfrom, QString text);
    void GetUpdatedDate(Section& , int& startfrom, QString text);
    void GetUrl(Section& , int& startfrom, QString text);
    QDateTime ConvertToDate(QString);
    void SkipPages(int);
    void timerEvent(QTimerEvent *);
    QNetworkAccessManager manager;

private:
    Ui::MainWindow *ui;
    int processedCount = 0;
    QString nextUrl;

public slots:
    void OnNetworkReply(QNetworkReply*);
private slots:
    void on_pbCrawl_clicked();

};

#endif // MAINWINDOW_H
