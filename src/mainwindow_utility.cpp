/*
FFSSE is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include "include/mainwindow.h"
#include "ui_mainwindow.h"

#include "Interfaces/authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/tags.h"

#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QPair>
#include <QPoint>
#include <QStringListModel>
#include <QDesktopServices>
#include <QTextCodec>
#include <QSettings>
#include <QMessageBox>

// ????? why not used

//QStringList MainWindow::GetNormalUrl(QString fandom)
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QString qs = QString("Select normal_url from fandoms where fandom = '%1' ").arg(fandom);
//    if(false)
//        qs+=" and (tracked = 1 or tracked_crossovers = 1)";
//    QSqlQuery q(qs, db);
//    QStringList result;
//    while(q.next())
//    {
//        QString lastPart = "/?&srt=1&lan=1&r=10&len=%1";
//        QSettings settings("settings.ini", QSettings::IniFormat);
//        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
//        int lengthCutoff = ui->cbWordCutoff->currentText() == "100k Words" ? 100 : 60;
//        lastPart=lastPart.arg(lengthCutoff);
//        QString resultString = "https://www.fanfiction.net" + q.value(0).toString() + lastPart;
//        result.push_back(resultString);
//        qDebug() << result;
//    }
//    return result;
//}

//QStringList MainWindow::GetCrossoverUrl(QString fandom)
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QString qs = QString("Select crossover_url from fandoms where fandom = '%1' ").arg(fandom);

//    QSqlQuery q(qs, db);
//    QStringList result;
//    while(q.next())
//    {
//        QString rebindName = q.value(0).toString();
//        QStringList temp = rebindName.split("/");

//        rebindName = "/" + temp.at(2) + "-Crossovers" + "/" + temp.at(3);
//        QString lastPart = "/0/?&srt=1&lan=1&r=10&len=%1";
//        QSettings settings("settings.ini", QSettings::IniFormat);
//        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
//        int lengthCutoff = ui->cbWordCutoff->currentText() == "100k Words" ? 100 : 60;
//        lastPart=lastPart.arg(lengthCutoff);
//        QString resultString =  "https://www.fanfiction.net" + rebindName + lastPart;
//        result.push_back(resultString);
//    }
//    qDebug() << result;
//    return result;
//}

void MainWindow::WipeSelectedFandom(bool)
{
    QString fandom;

    fandom = GetCurrentFandomName();

    if(!fandom.isEmpty())
    {
        QSqlDatabase db = QSqlDatabase::database();
        QString qs = QString("delete from fanfics where fandom like '%%1%'");
        qs=qs.arg(fandom);
        QSqlQuery q(qs, db);
        q.exec();
    }
}



//void MainWindow::UpdateFandomList(std::function<QString(core::Fandom)> linkGetter)
//{
//    for(auto value : sections)
//    {
//        currentProcessedSection = value.GetName();

//        An<PageManager> pager;
//        WebPage result = pager->GetPage(linkGetter(value), ECacheMode::dont_use_cache);
//        ProcessFandoms(result);

//        //manager.get(QNetworkRequest(QUrl(linkGetter(value))));
//        managerEventLoop.exec();
//    }
//}

//bool MainWindow::CheckSectionAvailability()
//{
//    auto fandomCount = fandomsInterface->GetFandomCount();
//    if(fandomCount == 0)
//    {
//        lblCurrentOperation->setText("Please, wait");
//        QMessageBox::information(nullptr, "Attention!", "Section information is not available, the app will now load it from the internet.\nThis is a one time operation, unless you want to update it with \"Reload section data\"\nPlease wait until it finishes before doing anything.");
//        ReInitFandoms();
//        QMessageBox::information(nullptr, "Attention!", "Section data is initialized, the app is usable. Have fun searching.");
//        pbMain->hide();
//        lblCurrentOperation->hide();
//    }
//    return true;
//}

//void MainWindow::ReInitFandoms()
//{
//    names.clear();
//    UpdateFandomList([](core::Fandom f){return f.url;});
//    UpdateFandomList([](core::Fandom f){return f.crossoverUrl;});
//    InsertFandomData(names);
//    ui->cbNormals->setModel(new QStringListModel(fandomsInterface->GetFandomList()));
//    ui->deCutoffLimit->setDate(QDateTime::currentDateTime().date());
//}




//void MainWindow::InsertFandomData(QMap<QPair<QString,QString>, core::Fandom> names)
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QHash<QPair<QString, QString>, core::Fandom> knownValues;
//    for(auto value : sections)
//    {

//        QString qs = QString("Select section, fandom, normal_url, crossover_url from fandoms where section = '%1'").
//                arg(value.name.replace("'","''"));
//        QSqlQuery q(qs, db);


//        while(q.next())
//        {
//            knownValues[{q.value("section").toString(),
//                    q.value("fandom").toString()}] =
//                    core::Fandom{q.value("fandom").toString(),
//                    q.value("section").toString(),
//                    q.value("normal_url").toString(),
//                    q.value("crossover_url").toString(),};
//        }
//        qDebug() << q.lastError();
//    }
//    auto make_key = [](core::Fandom f){return QPair<QString, QString>(f.section, f.name);} ;
//    pbMain->setMinimum(0);
//    pbMain->setMaximum(names.size());

//    int counter = 0;
//    QString prevSection;
//    for(auto fandom : names)
//    {
//        counter++;
//        if(prevSection != fandom.section)
//        {
//            lblCurrentOperation->setText("Currently loading: " + fandom.section);
//            prevSection = fandom.section;
//        }
//        auto key = make_key(fandom);
//        bool hasFandom = knownValues.contains(key);
//        if(!hasFandom)
//        {
//            QString insert = "INSERT INTO FANDOMS (FANDOM, NORMAL_URL, CROSSOVER_URL, SECTION) "
//                             "VALUES (:FANDOM, :URL, :CROSS, :SECTION)";
//            QSqlQuery q(db);
//            q.prepare(insert);
//            q.bindValue(":FANDOM",fandom.name.replace("'","''"));
//            q.bindValue(":URL",fandom.url.replace("'","''"));
//            q.bindValue(":CROSS",fandom.crossoverUrl.replace("'","''"));
//            q.bindValue(":SECTION",fandom.section.replace("'","''"));
//            q.exec();
//            if(q.lastError().isValid())
//                qDebug() << q.lastError();
//        }
//        if(hasFandom && (
//                    (knownValues[key].crossoverUrl.isEmpty() && !fandom.crossoverUrl.isEmpty())
//                    || (knownValues[key].url.isEmpty() && !fandom.url.isEmpty())))
//        {
//            QString insert = "UPDATE FANDOMS set normal_url = :normal, crossover_url = :cross "
//                             "where section = :section and fandom = :fandom";
//            QSqlQuery q(db);
//            q.prepare(insert);
//            q.bindValue(":fandom",fandom.name.replace("'","''"));
//            q.bindValue(":normal",fandom.url.replace("'","''"));
//            q.bindValue(":cross",fandom.crossoverUrl.replace("'","''"));
//            q.bindValue(":section",fandom.section.replace("'","''"));
//            q.exec();
//            if(q.lastError().isValid())
//                qDebug() << q.lastError();
//        }

//        if(counter%100 == 0)
//        {
//            pbMain->setValue(counter);
//            QApplication::processEvents();
//        }
//    }
//    pbMain->setValue(pbMain->maximum());
//    QApplication::processEvents();
//}


void MainWindow::OpenTagWidget(QPoint pos, QString url)
{
    url = url.replace(" none ", "");
    QStringList temp = url.split("TAGS");
    QString id = temp.at(0);
    QString tags= temp.at(1);

    QList<QPair<QString, QString>> tagPairs;

    for(QString tag: ui->wdgTagsPlaceholder->GetAllTags())
    {
        if(tags.contains(tag))
            tagPairs.push_back({"1", tag});
        else
            tagPairs.push_back({"0", tag});
    }

    tagWidgetDynamic->InitFromTags(id.toInt(), tagPairs);
    tagWidgetDynamic->resize(500,200);
    QPoint tempPoint(ui->edtResults->x(), 0);
    tempPoint = mapToGlobal(ui->twMain->mapTo(this, tempPoint));
    tagWidgetDynamic->move(tempPoint.x(), pos.y());
    tagWidgetDynamic->setWindowFlags(Qt::FramelessWindowHint);


    tagWidgetDynamic->show();
    tagWidgetDynamic->setFocus();
}

//void MainWindow::ProcessFandoms(WebPage webPage)
//{

//    QString str(webPage.content);
//    // getting to the start of fandom section
//    QRegExp rxStartFandoms("list_output");
//    int indexStart = rxStartFandoms.indexIn(str);
//    if(indexStart == -1)
//    {
//        QMessageBox::warning(0, "warning!", "failed to find the start of fandom section");
//        return;
//    }

//    QRegExp rxEndFandoms("</TABLE>");
//    int indexEnd= rxEndFandoms.indexIn(str);
//    if(indexEnd == -1)
//    {
//        QMessageBox::warning(0, "warning!", "failed to find the end of fandom section");
//        return;
//    }
//    while(true)
//    {
//        QRegExp rxStartLink("href=\"");
//        QRegExp rxEndLink("/\"");

//        int linkStart = rxStartLink.indexIn(str, indexStart);
//        if(linkStart == -1)
//            break;
//        int linkEnd= rxEndLink.indexIn(str, linkStart);
//        if(linkStart == -1 || linkEnd == -1)
//        {
//            QMessageBox::warning(0, "warning!", "failed to fetch link at: ", str.mid(linkStart, str.size() - linkStart));
//        }
//        QString link = str.mid(linkStart + rxStartLink.pattern().length(),
//                               linkEnd - (linkStart + rxStartLink.pattern().length()));

//        QRegExp rxStartName(">");
//        QRegExp rxEndName("</a");

//        int nameStart = rxStartName.indexIn(str, linkEnd);
//        int nameEnd= rxEndName.indexIn(str, nameStart);
//        if(nameStart == -1 || nameEnd == -1)
//        {
//            QMessageBox::warning(0, "warning!", "failed to fetch name at: ", str.mid(nameStart, str.size() - nameStart));
//        }
//        QString name = str.mid(nameStart + rxStartName.pattern().length(),
//                               nameEnd - (nameStart + rxStartName.pattern().length()));

//        //qDebug()  << name << " " << link << " " << counter++;
//        indexStart = linkEnd;
//        names.insert({currentProcessedSection,name}, core::Fandom{name, currentProcessedSection, link, ""});
//    }
//    managerEventLoop.quit();
//}

//void MainWindow::ProcessCrossovers(WebPage webPage)
//{
//    QString str(webPage.content);
//    //QString pattern = sections[currentProcessedSection].section;//.mid(indexOfSlash + 1, currentProcessedSection.length() - (indexOfSlash +1)) + "\\sCrossovers";
//    QRegExp rxStartFandoms("<TABLE\\sWIDTH='100%'><TR>");
//    int indexStart = rxStartFandoms.indexIn(str);
//    if(indexStart == -1)
//    {
//        QMessageBox::warning(0, "warning!", "failed to find the start of fandom section");
//        return;
//    }

//    QRegExp rxEndFandoms("</TABLE>");
//    int indexEnd= rxEndFandoms.indexIn(str);
//    if(indexEnd == -1)
//    {
//        QMessageBox::warning(0, "warning!", "failed to find the end of fandom section");
//        return;
//    }
//    while(true)
//    {
//        QRegExp rxStartLink("href=[\"]");
//        QRegExp rxEndLink("/\"");

//        int linkStart = rxStartLink.indexIn(str, indexStart);
//        if(linkStart == -1)
//            break;
//        int linkEnd= rxEndLink.indexIn(str, linkStart);
//        if(linkStart == -1 || linkEnd == -1)
//        {
//            QMessageBox::warning(0, "warning!", "failed to fetch link at: ", str.mid(linkStart, str.size() - linkStart));
//        }
//        QString link = str.mid(linkStart + rxStartLink.pattern().length()-2,
//                               linkEnd - (linkStart + rxStartLink.pattern().length())+2);

//        QRegExp rxStartName(">");
//        QRegExp rxEndName("</a");

//        int nameStart = rxStartName.indexIn(str, linkEnd);
//        int nameEnd= rxEndName.indexIn(str, nameStart);
//        if(nameStart == -1 || nameEnd == -1)
//        {
//            QMessageBox::warning(0, "warning!", "failed to fetch name at: ", str.mid(nameStart, str.size() - nameStart));
//        }
//        QString name = str.mid(nameStart + rxStartName.pattern().length(),
//                               nameEnd - (nameStart + rxStartName.pattern().length()));

//        indexStart = linkEnd;
//        if(!names.contains({currentProcessedSection,name}))
//            names.insert({currentProcessedSection,name},core::Fandom{name, currentProcessedSection,  "",link});
//        else
//            names[{currentProcessedSection,name}].crossoverUrl = link;
//    }
//    managerEventLoop.quit();
//}

//void MainWindow::on_pbInit_clicked()
//{
//    ReInitFandoms();
//}


// I don't like the idea of alive validation on this list creation
//seems wrong place to do it
//            QString url = "https://www.fanfiction.net/s/" + QString::number(ficPtr->webId);
//            auto page = pager->GetPage(url, ECacheMode::dont_use_cache);
//            if(page.content.contains("Unable to locate story."))
//            {
//                qDebug() << "skipping deleted story:" <<url;
//                continue;
//            }

//            QThread::msleep(500);

//authorsInterface->GetById(ficPtr->authorId);
