#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QRegExp>
#include <algorithm>




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(
     &manager, SIGNAL (finished(QNetworkReply*)),
     this, SLOT (OnNetworkReply(QNetworkReply*)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::RequestPage(QString page)
{
    if(page.isEmpty())
        page = ui->leUrl->text();
    QString origin = ui->leStartFromPage->text();

    QString pageCount = ui->lePageLimit->text();
    if(pageCount.isEmpty() || pageCount.toInt() < 0 || pageCount.toInt() > 1000)
    {
        if(QMessageBox::question(nullptr, "Warning!","Page count is questionable. Really use it?") != QMessageBox::Yes)
            return;
    }
    manager.get(QNetworkRequest(QUrl(ui->leUrl->text())));
}

void MainWindow::ProcessPage(QString str)
{
    Section section;
    int currentPosition = 0;
    while(true)
    {
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        currentPosition = section.end;
        GetUrl(section, currentPosition, str);
        GetTitle(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);
        GetWordCount(section, currentPosition, str);
        GetGenre(section, currentPosition, str);
        GetUpdatedDate(section, currentPosition, str);
        GetPublishedDate(section, currentPosition, str);
    }

}

void MainWindow::GetAuthor(Section & section, int& startfrom, QString text)
{
    QRegExp rxBy(QRegExp::escape("by"));
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexBy = rxBy.indexIn(text, startfrom);
    int indexStart = rxStart.indexIn(text, indexBy);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.author = text.mid(indexStart + 1,indexEnd - (indexStart + 1));

}

void MainWindow::GetTitle(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.title = text.mid(indexStart + 1,indexEnd - (indexStart + 1));
}

void MainWindow::GetGenre(Section & section, int& startfrom, QString text)
{
    //need to check if genre section even exists
    QString toCheck(text.mid(section.summaryEnd, section.wordCountStart - section.summaryEnd));
    int count = std::count_if(toCheck.begin(), toCheck.end(), [](QString val){return val == "-";});
    if(count == 3)
        return ;
    QRegExp rxStart(QRegExp::escape("-"));

    int indexStart = rxStart.indexIn(text, startfrom);
    indexStart = rxStart.indexIn(text, indexStart + 1);
    int indexEnd = rxStart.indexIn(text, indexStart + 1);
    section.genre = text.mid(indexStart,indexEnd - indexStart).trimmed();
    section.genre = section.genre .replace("/", "");
}

void MainWindow::GetSummary(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("padtop'>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;


}

void MainWindow::GetWordCount(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("-\\sWords:"));
    QRegExp rxEnd(QRegExp::escape("\\s-\\sReviews:"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    QString words = text.mid(indexStart + 9,indexEnd - (indexStart + 9));
    words=words.replace(" ", "");
    words=words.replace(",", "");
    section.wordCount = words.toInt();
    section.wordCountStart = indexStart;
}

void MainWindow::GetPublishedDate(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("<"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);
    startfrom = indexEnd;
    section.updated = ConvertToDate(text.mid(indexStart + 1,indexEnd - (indexStart + 1)));
}

void MainWindow::GetUpdatedDate(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("<"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);
    startfrom = indexEnd + 8;
    section.updated = ConvertToDate(text.mid(indexStart + 1,indexEnd - (indexStart + 1)));
}

void MainWindow::GetUrl(Section & section, int& startfrom, QString text)
{
    // looking for first href
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\"><img"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.url = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
}

QDateTime MainWindow::ConvertToDate(QString text)
{
    bool containsSpaces = text.trimmed().contains(" ");
    bool containsComma = text.trimmed().contains(",");
    QDateTime result;

    if( !containsSpaces)
    {
        //hours
        result = QDateTime::currentDateTime().addSecs(-1*60*60*text.replace(QRegExp("[\\sh]"), "").toInt());
    }
    if(containsSpaces && !containsComma)
    {
        //with month
        result = QDateTime::fromString(QString::number(QDateTime::currentDateTime().date().year()) + " " + text.trimmed() , "yyyy MMM d");
    }
    if(containsSpaces && !containsComma)
    {
        result = QDateTime::fromString(text.trimmed() , "MMM d, yyyy");
    }
    return result;
}

Section MainWindow::GetSection(QString text, int start)
{
    Section section;
    QRegExp rxStart(QRegExp::escape("<div\\sclass=\'z-list\\szhover\\szpointer\\s\'"));
    QRegExp rxEnd(QRegExp::escape("/div"));
    int index = rxStart.indexIn(text, start);
    if(index != -1)
    {
        section.isValid = true;
        section.start = index;
        section.end == rxEnd.indexIn(text, start);
    }
    return section;
}

void MainWindow::SkipPages(int)
{
    //intentionally blank
}

void MainWindow::timerEvent(QTimerEvent *)
{
    if(processedCount < ui->lePageLimit->text().toInt()
            && !nextUrl.isEmpty())
        manager.get(QNetworkRequest(QUrl(nextUrl)));
}

void MainWindow::OnNetworkReply(QNetworkReply * reply)
{

    ++processedCount;
    QByteArray data=reply->readAll();
    reply->deleteLater();
    QString str(data);
    ProcessPage(str);

    if(processedCount == ui->lePageLimit->text().toInt())
    {
        processedCount = 0;
        nextUrl = QString();
    }
}

void MainWindow::on_pbCrawl_clicked()
{
    processedCount = 0;
    nextUrl = QString();
    SkipPages(ui->leStartFromPage->text().toInt()-1);
    RequestPage(ui->leUrl->text());
}
