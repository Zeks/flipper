/*
Flipper is a recommendation and search engine for fanfiction.net
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
#include "include/ui/servitorwindow.h"
#include "include/Interfaces/genres.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"

#include "include/Interfaces/authors.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/Interfaces/ffn/ffn_fanfics.h"
#include "include/Interfaces/ffn/ffn_authors.h"
#include "include/url_utils.h"

#include "include/grpc/grpc_source.h"
#include "include/sqlitefunctions.h"

#include "include/tasks/slash_task_processor.h"
#include <QTextCodec>
#include <QSettings>
#include <QSqlRecord>
#include <QFuture>
#include <QVBoxLayout>
#include <QMutex>
#include <QtConcurrent>

#include <QFutureWatcher>
#include "ui_servitorwindow.h"
#include "include/parsers/ffn/ficparser.h"
#include "include/parsers/ffn/desktop_favparser.h"
#include "include/timeutils.h"
#include "include/page_utils.h"
#include "pagegetter.h"
#include "tasks/recommendations_reload_precessor.h"
#include <type_traits>
#include <algorithm>
//#include <QtCharts>
#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>

template <core::ERecDataType EnumType, typename ContainerType, typename InterfaceType>
void LoadDataForCalc(InterfaceType interface, ContainerType& container, QString storageFolder, QString suffix = "")
{
    QDir dir(QDir::currentPath());
    dir.mkdir("ServerData");

    QString ptrStr = QString("0x%1").arg((quintptr)&container,
                                         QT_POINTER_SIZE * 2, 16, QChar('0'));
    QLOG_INFO() << "loading with suffix: " << suffix << " to: " << ptrStr;

    QString fileBase;
    if(suffix.isEmpty())
        fileBase = QString::fromStdString(core::DataHolderInfo<EnumType>::fileBase());
    else
        fileBase = QString::fromStdString(core::DataHolderInfo<EnumType>::fileBase(suffix));

    QSettings settings("settings/settings_server.ini", QSettings::IniFormat);
    if(settings.value("Settings/usestoreddata", false).toBool() && QFile::exists(storageFolder + "/" +
                                                                                 fileBase
                                                                                 + "_0.txt"))
    {
        thread_boost::LoadData(storageFolder,
                               fileBase,
                               container);
    }
    else
    {
        container = core::DataHolderInfo<EnumType>::loadFunc()(interface);
        thread_boost::SaveData(storageFolder,
                               fileBase,
                               container);
    }
}

inline QString CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    auto result = server_address_proto.arg(ip,port);
    return result;
}
ServitorWindow::ServitorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::servitorWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<WebPage>("WebPage");
    qRegisterMetaType<PageResult>("PageResult");
    qRegisterMetaType<ECacheMode>("ECacheMode");
    //    qRegisterMetaType<FandomParseTask>("FandomParseTask");
    //    qRegisterMetaType<FandomParseTaskResult>("FandomParseTaskResult");
    ReadSettings();

    An<interfaces::GenreIndex> genreIndex;
    ui->cbGenres->blockSignals(true);

    genreList << "not found" << "Romance" << "Drama" << "Tragedy" << "Angst"
              << "Humor" << "Parody" << "Hurt/Comfort" << "Family" << "Friendship"
              << "Horror" << "Adventure" << "Mystery" << "Supernatural" <<  "Suspense" << "Sci-Fi"
              << "Fantasy" << "Spiritual" << "Western" << "Crime";


    for(auto i = 0; i < genreList.size(); i++)
        ui->cbGenres->addItem(genreList[i]);

    ui->cbGenres->blockSignals(false);

    ui->cbMoodSelector->blockSignals(true);

    moodList << "Neutral" << "Flirty" << "Funny" << "Dramatic" << "Hurty" << "Bondy" << "Shocky";


    for(auto i = 0; i < moodList.size(); i++)
        ui->cbMoodSelector->addItem(moodList[i]);

    ui->cbMoodSelector->blockSignals(false);
//    ui->tbrDiscords->setReadOnly(false);
    //ui->tbrDiscords->set
    CreateChartViews();

}

ServitorWindow::~ServitorWindow()
{
    WriteSettings();
    delete ui;
}

void ServitorWindow::ReadSettings()
{
    QSettings settings("settings/servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    ui->leFicUrl->setText(settings.value("Settings/ficUrl", "").toString());
    ui->leReprocessFics->setText(settings.value("Settings/reprocessIds", "").toString());
}

void ServitorWindow::WriteSettings()
{
    QSettings settings("settings/servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(!ui->leFicUrl->text().trimmed().isEmpty())
        settings.setValue("Settings/ficUrl", ui->leFicUrl->text());
    if(!ui->leReprocessFics->text().trimmed().isEmpty())
        settings.setValue("Settings/reprocessIds", ui->leReprocessFics->text());
    settings.sync();
}

void ServitorWindow::UpdateInterval(int, int)
{

}

void ServitorWindow::CreateChartView(ChartData & view, QWidget* widget, QStringList categories)
{
    view.chartView.reset(new QtCharts::QChartView());
    view.chart.reset(new QtCharts::QChart());
    view.chartView->setRenderHint(QPainter::Antialiasing);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(view.chartView.data());
    widget->setLayout(layout);

    QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
    axisX->append(categories);
    view.chart->addAxis(axisX, Qt::AlignBottom);
    view.axisY.reset(new QtCharts::QValueAxis());
    view.axisY->setRange(0,20000);
    view.chart->addAxis(view.axisY.data(), Qt::AlignLeft);
    view.chart->setTitle("Genre distributions");
    view.chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
}

void ServitorWindow::CreateChartViews()
{
    QStringList categoryNames;
    categoryNames << "0" << "0.05" << "0.1" <<
                     "0.15" << "0.2" <<
                     "0.25" << "0.3" <<
                     "0.35" << "0.4" <<
                     "0.45" << "0.5" <<
                     "0.55" << "0.6" <<
                     "0.65" << "0.7" <<
                     "0.75" << "0.8" <<
                     "0.85" << "0.9" <<
                     "0.95" << "1" << "1+";
    // genres
    CreateChartView(viewGenre, ui->wdgCharts, categoryNames);
    CreateChartView(viewMood, ui->wdgMoodChart, categoryNames);
    CreateChartView(viewListCompare, ui->wdgGenreCompare, genreList);
    CreateChartView(viewMoodCompare, ui->wdgMoodCompare, moodList);
    CreateChartView(viewMatchingList, ui->wdgMatchingList, moodList);
}

void ServitorWindow::InitGenreChartView(QString name)
{

    // first need to create the series
    An<interfaces::GenreIndex> genreIndex;
    auto genre = genreIndex->GenreByName(name);
    if(!genre.isValid)
    {
        QLOG_INFO() << "invalid genre!";
        return;
    }
    viewGenre.chart->removeAllSeries();

    auto index = genre.indexInDatabase;
    QList<double> categories;
    categories << 0 << 0.05 << 0.1 <<
                  0.15 << 0.2 <<
                  0.25 << 0.3 <<
                  0.35 << 0.4 <<
                  0.45 << 0.5 <<
                  0.55 << 0.6 <<
                  0.65 << 0.7 <<
                  0.75 << 0.8 <<
                  0.85 << 0.9 <<
                  0.95 << 1.001 << 100;



    QVector<int> countsOriginal;
    QVector<int> countsAdjusted;
    QtCharts::QBarSet *setOriginals = new QtCharts::QBarSet("Originals");
    QtCharts::QBarSet *setAdjusted = new QtCharts::QBarSet("Adjusted");
    int maxValue = 0;
    QLOG_INFO() << "Size of original author data: " << authorGenreDataOriginal.size();
    QLOG_INFO() << "Size of adjusted author data: " << iteratorProcessor.resultingGenreAuthorData.size();
    for(int i = 0; i < categories.size() - 1; i++)
    {
        auto value = std::count_if(std::begin(authorGenreDataOriginal), std::end(authorGenreDataOriginal), [categories,index, start = i, end = i+1](const std::array<double, 22>& data){
            return data[index] >= categories[start] && data[index] < categories[end];
        });
        countsOriginal.push_back(value);
        *setOriginals << value;
        if(value > maxValue)
            maxValue = value;
        value = std::count_if(std::begin(iteratorProcessor.resultingGenreAuthorData), std::end(iteratorProcessor.resultingGenreAuthorData), [categories,index, start = i, end = i+1](const std::array<double, 22>& data){
            return data[index] >= categories[start] && data[index] < categories[end];
        });
        countsAdjusted.push_back(value);
        *setAdjusted << value;
        if(value > maxValue)
            maxValue = value;
    }


    QLOG_INFO()  << "Original distribution: "<< countsOriginal;
    QLOG_INFO()  << "Adjusted distribution: "<< countsAdjusted;


    QtCharts::QBarSeries *series = new QtCharts::QBarSeries();
    series->append(setOriginals);
    //series->append(setAdjusted);
    viewGenre.chart->addSeries(series);
    viewGenre.chart->setGeometry(viewGenre.chartView->rect());
    viewGenre.chart->removeAxis(viewGenre.axisY.data());
    viewGenre.axisY.reset(new QtCharts::QValueAxis);
    viewGenre.axisY->setRange(0, maxValue*1.05);
    viewGenre.axisY->setTickCount(10);


    //    axisY->setTickCount(5);
    viewGenre.chart->setTitle(name + " distribution.");
    viewGenre.chartView->setChart(viewGenre.chart.data());
    viewGenre.chart->setAxisY(viewGenre.axisY.data(), series);

    //chartView->chart()->setAxisY(axisY, series);

}

void ServitorWindow::InitMoodChartView(QString name)
{
    An<interfaces::GenreIndex> genreIndex;
    viewMood.chart->removeAllSeries();
    auto moodValue = [](genre_stats::ListMoodData& data, QString name) -> float{
        if(name == "Neutral")
            return data.strengthNeutral;
        if(name == "Flirty")
            return data.strengthFlirty;
        if(name == "Funny")
            return data.strengthFunny;
        if(name == "Dramatic")
            return data.strengthDramatic;
        if(name == "Bondy")
            return data.strengthBondy;
        if(name == "Shocky")
            return data.strengthShocky;
        if(name == "Hurty")
            return data.strengthHurty;
        return 0;
    };
    //moodList << "Neutral" << "Flirty" << "Humorous" << "Dramatic" << "Hurty" << "Bondy" << "Shocky";
    QList<float> categories;
    //categories << -1 << 10.1f;
    categories << -1 << 0.05f << 0.1f <<
                  0.15f << 0.2f <<
                  0.25f << 0.3f <<
                  0.35f << 0.4f <<
                  0.45f << 0.5f <<
                  0.55f << 0.6f <<
                  0.65f << 0.7f <<
                  0.75f << 0.8f <<
                  0.85f << 0.9f <<
                  0.95f << 1.001f << 100.f;
    QVector<int> countsOriginal;
    QVector<int> countsAdjusted;
    QtCharts::QBarSet *setOriginals = new QtCharts::QBarSet("Originals");
    QtCharts::QBarSet *setAdjusted = new QtCharts::QBarSet("Adjusted");
    int maxValue = 0;
    QLOG_INFO()  << "Original distribution size: "<< originalMoodData.size();
    QLOG_INFO()  << "Adjusted distribution size: "<< adjustedMoodData.size();

    int sumOriginal = 0;
    int sumAdjusted = 0;
    for(int i = 0; i < categories.size() - 1; i++)
    {
        auto value = std::count_if(std::begin(originalMoodData), std::end(originalMoodData),
                                   [moodValue, categories,name, start = i, end = i+1](genre_stats::ListMoodData& data){
            return moodValue(data, name) >= categories[start] && moodValue(data, name) < categories[end];
        });
        countsOriginal.push_back(value);
        sumOriginal+=value;
        *setOriginals << value;
        if(value > maxValue)
            maxValue = value;
        value = std::count_if(std::begin(adjustedMoodData), std::end(adjustedMoodData),
                              [moodValue, categories,name, start = i, end = i+1](genre_stats::ListMoodData& data){
            return moodValue(data, name) >= categories[start] && moodValue(data, name) < categories[end];
        });
        countsAdjusted.push_back(value);
        sumAdjusted+=value;
        *setAdjusted << value;
        if(value > maxValue)
            maxValue = value;
    }
    QLOG_INFO()  << "Original sum: "<< sumOriginal;
    QLOG_INFO()  << "Adjusted sum: "<< sumAdjusted;

    QLOG_INFO()  << "Original distribution: "<< countsOriginal;
    QLOG_INFO()  << "Adjusted distribution: "<< countsAdjusted;


    QtCharts::QBarSeries *series = new QtCharts::QBarSeries();
    series->append(setOriginals);
    series->append(setAdjusted);
    viewMood.chart->addSeries(series);
    viewMood.chart->setGeometry(viewMood.chartView->rect());
    viewMood.chart->removeAxis(viewMood.axisY.data());
    viewMood.axisY.reset(new QtCharts::QValueAxis);
    viewMood.axisY->setRange(0, maxValue*1.2);
    viewMood.axisY->setTickCount(10);


    //    axisY->setTickCount(5);
    viewMood.chart->setTitle(name + " distribution.");
    viewMood.chartView->setChart(viewMood.chart.data());
    viewMood.chart->setAxisY(viewMood.axisY.data(), series);
}

void ServitorWindow::InitGenreCompareChartView(QList<int> users)
{
    viewListCompare.chart->removeAllSeries();
    An<interfaces::GenreIndex> genreIndex;

    QList<QtCharts::QBarSet *> userSets;
    for(auto i = 0; i < users.size(); i++)
    {
        if(i == 0)
            userSets.push_back(new QtCharts::QBarSet("Me"));
        else
            userSets.push_back(new QtCharts::QBarSet("User" + QString::number(i)));
    }

    for(int i =0 ; i < genreList.size(); i++)
    {
        auto index = genreIndex->genresByName[genreList[i]].indexInDatabase;
        for(auto u = 0; u < users.size(); u++)
            *userSets[u] << authorGenreDataOriginal[users[u]][index];

    }
    QtCharts::QBarSeries *series = new QtCharts::QBarSeries();
    for(auto u = 0; u < users.size(); u++)
        series->append(userSets[u]);

    viewListCompare.chart->addSeries(series);
    viewListCompare.chart->setGeometry(viewListCompare.chartView->rect());
    viewListCompare.chart->removeAxis(viewListCompare.axisY.data());
    viewListCompare.axisY.reset(new QtCharts::QValueAxis);
    viewListCompare.axisY->setRange(0, 0.8);
    viewListCompare.axisY->setTickCount(5);


    //    axisY->setTickCount(5);
    viewMood.chart->setTitle("Genre compare");
    viewListCompare.chartView->setChart(viewListCompare.chart.data());
    viewListCompare.chart->setAxisY(viewListCompare.axisY.data(), series);
}

void ServitorWindow::InitMoodCompareChartView(QList<int> users, bool useOriginalMoods)
{
    QLOG_INFO() << "InitMoodCompareChartView";
    viewMoodCompare.chart->removeAllSeries();

    QList<QtCharts::QBarSet *> userSets;
    for(auto i = 0; i < users.size(); i++)
    {
        if(i == 0)
            userSets.push_back(new QtCharts::QBarSet("Me"));
        else
        {
            auto ratio = QString::number(matchesForUsers[users[i]].ratio);
            auto ratioWithoutIgnores = QString::number(matchesForUsers[users[i]].ratioWithoutIgnores);
            userSets.push_back(new QtCharts::QBarSet("User" + QString::number(users[i])
                                                     + "(" + ratio + "/" + ratioWithoutIgnores + ")"));
        }
    }
    QLOG_INFO() << "InitMoodCompareChartView 2";
    userSets.push_back(new QtCharts::QBarSet("Diff"));
    QLOG_INFO() << "InitMoodCompareChartView 3";

    genre_stats::ListMoodData moodSumData;
    for(int i =0 ; i < moodList.size(); i++)
    {

        auto myValue = interfaces::Genres::ReadMoodValue(moodList[i], adjustedListMoodData);
        if(i == 1)
            QLOG_INFO() << "My value: " << myValue;

        for(auto u = 0; u < users.size(); u++)
        {
            if(u == 0)
                continue;
            auto theirValue = interfaces::Genres::ReadMoodValue(moodList[i], adjustedMoodData[users[u]]);
            auto diff = std::abs(std::max(myValue,theirValue) - std::min(myValue,theirValue));
            interfaces::Genres::WriteMoodValue(moodList[i],
                                               diff,
                                               moodSumData);
            if(i == 1)
                QLOG_INFO() << "Their value: " << theirValue << " Diff: " << diff << " Sum: " << interfaces::Genres::ReadMoodValue(moodList[i], moodSumData);

        }
    }
    QLOG_INFO() << "1";
    float sum = 0, sumMeaningful = 0;
    genre_stats::ListMoodData averageDiffData;
    for(int i =0 ; i < moodList.size(); i++)
    {
        auto value = interfaces::Genres::ReadMoodValue(moodList[i], moodSumData);
        auto divided = value/static_cast<float>(users.size()-1);
        interfaces::Genres::WriteMoodValue(moodList[i],
                                           divided,
                                           averageDiffData);
        sum  += divided;
        if(moodList[i] != "Neutral" && moodList[i] != "Funny")
            sumMeaningful  += divided;
    }
    QLOG_INFO() << "2";
    ui->leFinalDiff->setText(QString::number(sum));
    ui->leMeaningfulMoodDiff->setText(QString::number(sumMeaningful));


    for(int i =0 ; i < moodList.size(); i++)
    {
        for(auto u = 0; u < users.size(); u++)
        {
            if(useOriginalMoods)
            {
                if(!ui->edtLog->toPlainText().isEmpty() && u == 0)
                    *userSets[u] << interfaces::Genres::ReadMoodValue(moodList[i], listMoodData);
                else
                    *userSets[u] << interfaces::Genres::ReadMoodValue(moodList[i], originalMoodData[users[u]]);

            }
            else
            {
                if(!ui->edtLog->toPlainText().isEmpty() && u == 0)
                    *userSets[u] << interfaces::Genres::ReadMoodValue(moodList[i], adjustedListMoodData);
                else
                    *userSets[u] << interfaces::Genres::ReadMoodValue(moodList[i], adjustedMoodData[users[u]]);
            }
        }
        *userSets[users.size()] << interfaces::Genres::ReadMoodValue(moodList[i], averageDiffData);
    }

    QLOG_INFO() << "3";

    QtCharts::QBarSeries *series = new QtCharts::QBarSeries();
    for(auto u = 0; u < userSets.size(); u++)
        series->append(userSets[u]);
    QLOG_INFO() << "4";
    viewMoodCompare.chart->addSeries(series);
    viewMoodCompare.chart->setGeometry(viewMoodCompare.chartView->rect());
    viewMoodCompare.chart->removeAxis(viewMoodCompare.axisY.data());
    viewMoodCompare.axisY.reset(new QtCharts::QValueAxis);
    viewMoodCompare.axisY->setRange(0, 1);
    viewMoodCompare.axisY->setTickCount(20);


    //    axisY->setTickCount(5);
    viewMoodCompare.chart->setTitle("Mood compare");
    viewMoodCompare.chartView->setChart(viewMoodCompare.chart.data());
    viewMoodCompare.chart->setAxisY(viewMoodCompare.axisY.data(), series);
}

void ServitorWindow::InitMatchingListChartView(genre_stats::ListMoodData currentUserData,genre_stats::ListMoodData combinedData)
{
    viewMatchingList.chart->removeAllSeries();

    QList<QtCharts::QBarSet *> userSets;
    userSets.push_back(new QtCharts::QBarSet("User Data"));
    userSets.push_back(new QtCharts::QBarSet("Combined Data"));


    for(int i =0 ; i < moodList.size(); i++)
    {
        *userSets[0] << interfaces::Genres::ReadMoodValue(moodList[i], currentUserData);
        *userSets[1] << interfaces::Genres::ReadMoodValue(moodList[i], combinedData);
    }

    QtCharts::QBarSeries *series = new QtCharts::QBarSeries();
    for(auto u = 0; u < userSets.size(); u++)
        series->append(userSets[u]);

    viewMatchingList.chart->addSeries(series);
    viewMatchingList.chart->setGeometry(viewMatchingList.chartView->rect());
    viewMatchingList.chart->removeAxis(viewMatchingList.axisY.data());
    viewMatchingList.axisY.reset(new QtCharts::QValueAxis);
    viewMatchingList.axisY->setRange(0, 1);
    viewMatchingList.axisY->setTickCount(5);


    //    axisY->setTickCount(5);
    viewMatchingList.chart->setTitle("Moods for matchlist");
    viewMatchingList.chartView->setChart(viewMatchingList.chart.data());
    viewMatchingList.chart->setAxisY(viewMatchingList.axisY.data(), series);
}

void ServitorWindow::InitGrpcSource()
{
    if(!grpcSource)
    {
        QSettings settings("settings/settings.ini", QSettings::IniFormat);

        auto ip = settings.value("Settings/serverIp", "127.0.0.1").toString();
        auto port = settings.value("Settings/serverPort", "3055").toString();

       grpcSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), env.userToken,  160));
       grpcSource->userToken = env.userToken;
    }
}

QVector<genre_stats::FicGenreData> ServitorWindow::CreateGenreDataForFics(GenreDetectionSources input,
                                                                          CutoffControls cutoff,
                                                                          bool userIterationForGenreProcessing, bool displayLog){
    QVector<genre_stats::FicGenreData> ficGenreDataList;
    ficGenreDataList.reserve(input.ficsToUse.size());
    interfaces::GenreConverter genreConverter;
    int counter = 0;

    for(auto i = input.ficsToUse.begin(); i != input.ficsToUse.end(); i++)
    {
        auto key = i.key();
        const auto& value = i.value();
        if(!ui->leFicIdForGenre->text().isEmpty() && i.key() != ui->leFicIdForGenre->text().toInt())
            continue;

        if(counter%10000 == 0)
            qDebug() << "processing fic: " << counter;

        //gettting amount of funny lists
        int64_t funny = std::count_if(std::begin(value), std::end(value), [&](int listId){
            return input.moodAuthorLists[listId].strengthFunny >= cutoff.funny;
        });
        int64_t flirty = std::count_if(value.begin(), value.end(), [&](int listId){
            return input.moodAuthorLists[listId].strengthFlirty >= cutoff.flirty;
        });
        auto listSet = value;
        int64_t neutralAdventure = 0;
        for(auto listId : listSet)
            if(input.genreAuthorLists[listId][3] >= cutoff.adventure)
                neutralAdventure++;

        //qDebug() << "Adventure list count: " << neutralAdventure;

        int64_t hurty = std::count_if(value.begin(), value.end(), [&](int listId){
            return input.moodAuthorLists[listId].strengthHurty >= cutoff.hurty;
        });
        int64_t bondy = std::count_if(value.begin(), value.end(), [&](int listId){
            return input.moodAuthorLists[listId].strengthBondy >= cutoff.bonds;
        });

        int64_t neutral = std::count_if(value.begin(), value.end(), [&](int listId){
            return input.moodAuthorLists[listId].strengthNeutral>= cutoff.adventure;
        });
        int64_t dramatic = std::count_if(value.begin(), value.end(), [&](int listId){
            return input.moodAuthorLists[listId].strengthDramatic >= cutoff.drama;
        });

        int64_t totalUsersForFic = value.size();

        genre_stats::FicGenreData genreData;
        genreData.ficId = key;
        genreData.originalGenres =  genreConverter.GetFFNGenreList(input.originalFicGenres[key]);
        genreData.totalLists = static_cast<int>(totalUsersForFic);
        genreData.strengthHumor = static_cast<float>(funny)/static_cast<float>(totalUsersForFic);
        genreData.strengthRomance = static_cast<float>(flirty)/static_cast<float>(totalUsersForFic);
        genreData.strengthDrama = static_cast<float>(dramatic)/static_cast<float>(totalUsersForFic);
        genreData.strengthBonds = static_cast<float>(bondy)/static_cast<float>(totalUsersForFic);
        genreData.strengthHurtComfort = static_cast<float>(hurty)/static_cast<float>(totalUsersForFic);
        genreData.strengthNeutralComposite = static_cast<float>(neutral)/static_cast<float>(totalUsersForFic);
        genreData.strengthNeutralAdventure = static_cast<float>(neutralAdventure)/static_cast<float>(totalUsersForFic);
        if(displayLog)
            genreData.Log();
        if(!userIterationForGenreProcessing)
            genreConverter.ProcessGenreResult(genreData);
        else
            genreConverter.ProcessGenreResultIteration2(genreData);
        ficGenreDataList.push_back(genreData);
        counter++;
    }


    for(auto& fic : ficGenreDataList)
    {
        QStringList keptList;
        for(const auto& genre: std::as_const(fic.processedGenres))
        {
            if(genre.relevance < 0.1f)
                keptList += genre.genres;
        }

        for(const auto& genre : std::as_const(fic.processedGenres))
        {
            QString writtenGenre = genre.genres.join(",");
            if(genre.relevance > fic.maxGenrePercent && !writtenGenre.isEmpty())
            {
//                if(genre.relevance > 0)
//                    qDebug() << "Filling max_genre_percent: " << genre.relevance;
                fic.maxGenrePercent = genre.relevance;
            }
        }
        fic.keptToken = keptList.join(",");
    }

    return ficGenreDataList;
}


void ServitorWindow::DetectGenres(size_t minAuthorRecs, size_t minFoundLists)
{
    interfaces::GenreConverter converter;



    //QVector<int> ficIds;
    auto db = sql::Database::database();
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authors= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    genres->db = db;
    fanfics->db = db;
    authors->db = db;

    LoadDataForCalc<core::rdt_favourites,
            InputForGenresIteration2::FavType,
            QSharedPointer<interfaces::Authors>>(authors, inputs.faves, "TempData");

    qDebug() << "Finished list load";

    QHash<int, QSet<int>> ficsToUse;
    auto& faves  = inputs.faves;

    for(auto i = faves.begin(); i != faves.end(); i++)
    {
        const auto& set = i.value();
        if(set.cardinality() < minAuthorRecs)
            continue;
        for(auto fic : set)
        {
            ficsToUse[fic].insert(i.key());
        }
    }
    qDebug() << "Finished author processing, resulting set is of size:" << ficsToUse.size();



    QList<int> result;
    for(auto i = ficsToUse.begin(); i != ficsToUse.end(); i++)
    {
        if(i.value().size() >= static_cast<int>(minFoundLists))
            result.push_back(i.key());

    }

    QHash<int, QSet<int>> filteredFicsToUse;

    for(auto i = ficsToUse.begin(); i != ficsToUse.end(); i++)
    {
        if(i.value().size() >= static_cast<int>(minFoundLists))
            filteredFicsToUse[i.key()] = i.value();

    }

    qDebug() << "Finished counts";

    auto genreLists = authors->GetListGenreData();
    qDebug() << "got genre lists, size: " << genreLists.size();

    auto genresForFics = fanfics->GetGenreForFics();
    qDebug() << "collected genres for fics, size: " << genresForFics.size();
    auto moodLists = authors->GetMoodDataForLists();
    qDebug() << "got mood lists, size: " << moodLists.size();

    //auto genreRedirects = CreateGenreRedirects();

    QVector<genre_stats::FicGenreData> ficGenreDataList =
            CreateGenreDataForFics({genreLists,
                                    moodLists,
                                    genresForFics,
                                    filteredFicsToUse}, CutoffControls{});

    sql::Transaction transaction(db);
    if(!genres->WriteDetectedGenres(ficGenreDataList))
    {

        qDebug() << "cancelling transaction";
        transaction.cancel();
    }

    qDebug() << "finished writing genre data for fics";
    transaction.finalize();
    qDebug() << "Starting the second iteration";

}


//        int64_t pureDrama = std::count_if(genreLists[fic].begin(), genreLists[fic].end(), [&](int listId){
//            return genreLists[listId][static_cast<size_t>(genreRedirects["Drama"])] - genreLists[listId][static_cast<size_t>(genreRedirects["Romance"])] >= 0.05;
//        });
//        int64_t pureRomance = std::count_if(genreLists[fic].begin(), genreLists[fic].end(), [&](int listId){
//            return genreLists[listId][static_cast<size_t>(genreRedirects["Romance"])] - genreLists[listId][static_cast<size_t>(genreRedirects["Drama"])] >= 0.8;
//        });



template <core::ERecDataType EnumType, typename ContainerType>
void SaveDataForCalc(ContainerType& container, QString storageFolder, QString suffix = "")
{
    QString fileBase;
    if(suffix.isEmpty())
        fileBase = QString::fromStdString(core::DataHolderInfo<EnumType>::fileBase());
    else
        fileBase = QString::fromStdString(core::DataHolderInfo<EnumType>::fileBase(suffix));
    //QDir dir(QDir::currentPath());
    if(fileBase.isEmpty())
        return;
    //    dir.remove(fileBase + "*");
    thread_boost::SaveData(storageFolder,
                           fileBase,
                           container);
}


void ServitorWindow::CreateAdjustedGenresForAuthors()
{
    InputForGenresIteration2 inputs;


    interfaces::GenreConverter converter;

    //QVector<int> ficIds;
    auto db = sql::Database::database();
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authors= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    genres->db = db;
    fanfics->db = db;
    authors->db = db;

    LoadDataForCalc<core::rdt_fic_genres_composite,
            InputForGenresIteration2::FicGenreCompositeType,
            QSharedPointer<interfaces::Genres>>(genres, inputs.ficGenresComposite, "TempData");
    LoadDataForCalc<core::rdt_favourites,
            InputForGenresIteration2::FavType,
            QSharedPointer<interfaces::Authors>>(authors, inputs.faves, "TempData");

    auto& faves  = inputs.faves;
    QLOG_INFO() << "Loaded faves of size: " << inputs.faves.size();

    //return;
    authorGenreDataOriginal = authors->GetListGenreData();
    SaveDataForCalc<core::rdt_author_genre_distribution,
            QHash<int, std::array<double, 22>>>(authorGenreDataOriginal, "TempData", "original");


    qDebug() << "Starting the second iteration";
    iteratorProcessor.ReprocessGenreStats(inputs.ficGenresComposite, faves);
    SaveDataForCalc<core::rdt_author_genre_distribution,
            QHash<int, std::array<double, 22>>>(iteratorProcessor.resultingGenreAuthorData, "TempData", "adjusted");
    qDebug() << "finished iteration";
    //return;
    QLOG_INFO() << "Size of original author data: " << authorGenreDataOriginal.size();
    QLOG_INFO() << "Size of adjusted author data: " << iteratorProcessor.resultingGenreAuthorData.size();

    originalMoodData = iteratorProcessor.CreateMoodDataFromGenres(authorGenreDataOriginal);
    adjustedMoodData = iteratorProcessor.resultingMoodAuthorData;


    qDebug() << "Finished queue set";
}
void ServitorWindow::CreateSecondIterationOfGenresForFics(size_t minAuthorRecs, size_t minFoundLists)
{
    InputForGenresIteration2 inputs;
    //QVector<int> ficIds;
    auto db = sql::Database::database();
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authors= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    genres->db = db;
    fanfics->db = db;
    authors->db = db;

//    LoadDataForCalc<core::rdt_fic_genres_composite,
//            InputForGenresIteration2::FicGenreCompositeType,
//            QSharedPointer<interfaces::Genres>>(genres, inputs.ficGenresComposite, "TempData");
    LoadDataForCalc<core::rdt_favourites,
            InputForGenresIteration2::FavType,
            QSharedPointer<interfaces::Authors>>(authors, inputs.faves, "TempData");
//    LoadDataForCalc<core::rdt_fic_genres_original,
//            InputForGenresIteration2::FicGenreOriginalType,
//            QSharedPointer<interfaces::Fanfics>>(fanfics, inputs.ficGenresOriginal, "TempData");



    QHash<int, QSet<int>> ficsToUse;
    auto& faves  = inputs.faves;

    for(auto i = faves.begin(); i != faves.end(); i++)
    {
        const auto& set = i.value();
        if(set.cardinality() < minAuthorRecs)
            continue;
        for(auto fic : set)
            ficsToUse[fic].insert(i.key());
    }
    qDebug() << "Finished author processing, resulting set is of size:" << ficsToUse.size();

    QHash<int, QSet<int>> filteredFicsToUse;
    for(auto i = ficsToUse.begin(); i != ficsToUse.end(); i++)
    {
        if(i.value().size() >= static_cast<int>(minFoundLists))
        {
            const auto key = i.key();
            filteredFicsToUse[key] = i.value();
            inputs.filteredFicGenresOriginal[key] = inputs.ficGenresOriginal[key];
            inputs.filteredFicGenresComposite[key] = inputs.ficGenresComposite[key];
        }
    }
    QLOG_INFO() << "////////////// ORIGINAL DATA /////////////////";

    QVector<genre_stats::FicGenreData> ficGenreDataList = CreateGenreDataForFics({authorGenreDataOriginal,
                                                                                  originalMoodData,
                                                                                  inputs.ficGenresOriginal,
                                                                                  filteredFicsToUse}, CutoffControls{}, false, ui->chkLogFic->isChecked());
    QLOG_INFO() << "////////////// ADJUSTED DATA /////////////////";
    CutoffControls customCutoff;
//    customCutoff.funny = 0.15;
//    customCutoff.adventure = 0.2;
//    customCutoff.flirty = 0.45;
//    customCutoff.drama = 0.2;
//    customCutoff.hurty = 0.12;
//    customCutoff.bonds = 0.3;
    ficGenreDataList = CreateGenreDataForFics({iteratorProcessor.resultingGenreAuthorData,
                                                                                  adjustedMoodData,
                                                                                  inputs.ficGenresOriginal,
                                                                                  filteredFicsToUse}, customCutoff ,false, ui->chkLogFic->isChecked());
    database::Transaction transaction(db);

    if(!genres->WriteDetectedGenresIteration2(ficGenreDataList))
        transaction.cancel();
    else
        transaction.finalize();
    qDebug() << "finished writing genre data for fics";

}
void ServitorWindow::on_pbLoadGenreDistributions_clicked()
{
    auto db = sql::Database::database();
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authors= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    genres->db = db;
    fanfics->db = db;
    authors->db = db;
    authorGenreDataOriginal.clear();
    iteratorProcessor.resultingGenreAuthorData.clear();

    LoadDataForCalc<core::rdt_author_genre_distribution,
            QHash<int, std::array<double, 22>>,
            QSharedPointer<interfaces::Authors>>(authors, iteratorProcessor.resultingGenreAuthorData, "TempData", "adjusted");

    LoadDataForCalc<core::rdt_author_genre_distribution,
            QHash<int, std::array<double, 22>>,
            QSharedPointer<interfaces::Authors>>(authors, authorGenreDataOriginal, "TempData", "original");
    originalMoodData = iteratorProcessor.CreateMoodDataFromGenres(authorGenreDataOriginal);
    adjustedMoodData = iteratorProcessor.CreateMoodDataFromGenres(iteratorProcessor.resultingGenreAuthorData);

}

void ServitorWindow::on_pbLoadFic_clicked()
{
    //    PageManager pager;
    //    FicParser parser;
    //    QHash<QString, int> fandoms;
    //    auto result = database::GetAllFandoms(fandoms);
    //    if(!result)
    //        return;
    //    QString url = ui->leFicUrl->text();
    //    auto page = pager.GetPage(url, ECacheMode::use_cache);
    //    parser.SetRewriteAuthorNames(false);
    //    parser.ProcessPage(url, page.content);
    //    parser.WriteProcessed(fandoms);
}

void ServitorWindow::on_pbReprocessFics_clicked()
{
    //    PageManager pager;
    //    FicParser parser;
    //    QHash<QString, int> fandoms;
    //    auto result = database::GetAllFandoms(fandoms);
    //    if(!result)
    //        return;
    //    sql::Database db = sql::Database::database();
    //    //db.transaction();
    //    database::ReprocessFics(" where fandom1 like '% CROSSOVER' and alive = 1 order by id asc", "ffn", [this,&pager, &parser, &fandoms](int ficId){
    //        //todo, get web_id from fic_id
    //        QString url = url_utils::GetUrlFromWebId(webId, "ffn");
    //        auto page = pager.GetPage(url, ECacheMode::use_only_cache);
    //        parser.SetRewriteAuthorNames(false);
    //        auto fic = parser.ProcessPage(url, page.content);
    //        if(fic.isValid)
    //            parser.WriteProcessed(fandoms);
    //    });
}

void ServitorWindow::on_pushButton_clicked()
{
    //database::EnsureFandomsNormalized();
}

void ServitorWindow::on_pbGetGenresForFic_clicked()
{

}

void ServitorWindow::on_pbGetGenresForEverything_clicked()
{
    DetectGenres(25,15);
}

void ServitorWindow::on_pbGenresIteration2_clicked()
{
    CreateAdjustedGenresForAuthors();
    //25, 15
}

void ServitorWindow::on_pushButton_2_clicked()
{
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanfics->db = sql::Database::database();
    fanfics->ResetActionQueue();
    QSettings settings("settings/settings_servitor.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.setValue("Settings/genrequeued", false);
    settings.sync();
}

void ServitorWindow::on_pbGetData_clicked()
{
    An<PageManager> pager;
    auto data = pager->GetPage(ui->leGetCachedData->text(), fetching::CacheStrategy::CacheOnly());
    ui->edtLog->clear();
    ui->edtLog->insertPlainText(data.content);

    // need to get:
    // last date of published favourite fic
    // amount of favourite fics at the moment of last parse
    // (need to keep this to check if list is updated even if last published is teh same)
}

void ServitorWindow::on_pushButton_3_clicked()
{
    auto db = sql::Database::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanfics->db = db;
    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    authorInterface->db = db;
    auto authors = authorInterface->GetAllAuthorsLimited("ffn", 0);


    An<PageManager> pager;

    auto job = [fanfics, authorInterface, fandomInterface](QString url, QString content){
        FavouriteStoryParser parser;
        parser.ProcessPage(url, content);
        return parser;
    };

    QVector<QFuture<FavouriteStoryParser>> futures;
    QList<FavouriteStoryParser> parsers;



    database::Transaction transaction(db);
    WebPage data;
    int counter = 0;
    for(const auto& author : authors)
    {
        if(counter%1000 == 0)
            QLOG_INFO() <<  counter;

        futures.clear();
        parsers.clear();
        //QLOG_INFO() <<  "Author: " << author->url("ffn");
        FavouriteStoryParser parser;
        parser.authorName = author->name;

        //TimedAction pageAction("Page loaded in: ",[&](){
        data = pager->GetPage(author->url("ffn"), fetching::CacheStrategy::CacheOnly());
        //});
        //pageAction.run();

        //TimedAction pageProcessAction("Page processed in: ",[&](){
        auto splittings = page_utils::SplitJob(data.content);

        for(const auto& part: std::as_const(splittings.parts))
        {
            futures.push_back(QtConcurrent::run(job, data.url, part.data));
        }
        for(auto future: futures)
        {
            future.waitForFinished();
            parsers+=future.result();
        }

        //});
        //pageProcessAction.run();

        //QSet<QString> fandoms;
        FavouriteStoryParser::MergeStats(author, fandomInterface, parsers);
        //TimedAction action("Author updated in: ",[&](){
        authorInterface->UpdateAuthorFavouritesUpdateDate(author);
        //});
        //action.run();
        //QLOG_INFO() <<  "Author: " << author->url("ffn") << " Update date: " << author->stats.favouritesLastUpdated;
        counter++;

    }
    transaction.finalize();

}

void ServitorWindow::on_pbUpdateFreshAuthors_clicked()
{
    auto db = sql::Database::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;

    auto authors = authorInterface->GetAllAuthorsWithFavUpdateSince("ffn", QDateTime::currentDateTime().addMonths(-24));

    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->authorInterface = authorInterface;



    RecommendationsProcessor reloader(db, fanficsInterface,
                                      fandomInterface,
                                      authorInterface,
                                      recsInterface);

    connect(&reloader, &RecommendationsProcessor::resetEditorText, this,    &ServitorWindow::OnResetTextEditor);
    connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &ServitorWindow::OnProgressBarRequested);
    connect(&reloader, &RecommendationsProcessor::updateCounter, this,      &ServitorWindow::OnUpdatedProgressValue);
    connect(&reloader, &RecommendationsProcessor::updateInfo, this,         &ServitorWindow::OnNewProgressString);


    reloader.SetStagedAuthors(authors);
    fetching::CacheStrategy cacheStrategy;
    cacheStrategy.useCache = true;
    cacheStrategy.cacheExpirationDays = 7;
    cacheStrategy.fetchIfCacheIsOld = true;
    cacheStrategy.fetchIfCacheUnavailable = true;
    reloader.ReloadRecommendationsList(cacheStrategy);

}

void ServitorWindow::OnResetTextEditor()
{

}

void ServitorWindow::OnProgressBarRequested()
{

}

void ServitorWindow::OnUpdatedProgressValue(int )
{

}

void ServitorWindow::OnNewProgressString(QString )
{

}

void ServitorWindow::on_pbUpdateInterval_clicked()
{
    auto db = sql::Database::database();
    auto isOpen = db.isOpen();
    Q_UNUSED(isOpen);
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;

    auto authors = authorInterface->GetAllAuthorsWithFavUpdateBetween("ffn",
                                                                      QDateTime::currentDateTime().addMonths(-1*ui->sbUpdateIntervalStart->value()),
                                                                      QDateTime::currentDateTime().addMonths(-1*ui->sbUpdateIntervalEnd->value())
                                                                      );



    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->authorInterface = authorInterface;



    RecommendationsProcessor reloader(db, fanficsInterface,
                                      fandomInterface,
                                      authorInterface,
                                      recsInterface);

    connect(&reloader, &RecommendationsProcessor::resetEditorText, this,    &ServitorWindow::OnResetTextEditor);
    connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &ServitorWindow::OnProgressBarRequested);
    connect(&reloader, &RecommendationsProcessor::updateCounter, this,      &ServitorWindow::OnUpdatedProgressValue);
    connect(&reloader, &RecommendationsProcessor::updateInfo, this,         &ServitorWindow::OnNewProgressString);


    reloader.SetStagedAuthors(authors);

    fetching::CacheStrategy customCacheStrategy;
    if(ui->cbCacheMode->currentIndex() == 0){
        customCacheStrategy = fetching::CacheStrategy::NetworkOnly();
    }
    else if(ui->cbCacheMode->currentIndex() == 1){
        customCacheStrategy.useCache = true;
        customCacheStrategy.cacheExpirationDays = 7;
        customCacheStrategy.fetchIfCacheIsOld = true;
        customCacheStrategy.fetchIfCacheUnavailable = true;
        }
    else{
        customCacheStrategy = fetching::CacheStrategy::CacheOnly();
    }

    reloader.ReloadRecommendationsList(customCacheStrategy);
    QLOG_INFO() << "FINISHED";
}

void ServitorWindow::on_pbReprocessAllFavPages_clicked()
{
    auto db = sql::Database::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;

    QList<core::AuthorPtr> authors;
    {
        database::Transaction transaction(db);
        authors = authorInterface->GetAllAuthors("ffn");
    }


    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->authorInterface = authorInterface;



    RecommendationsProcessor reloader(db, fanficsInterface,
                                      fandomInterface,
                                      authorInterface,
                                      recsInterface);

    connect(&reloader, &RecommendationsProcessor::resetEditorText, this,    &ServitorWindow::OnResetTextEditor);
    connect(&reloader, &RecommendationsProcessor::requestProgressbar, this, &ServitorWindow::OnProgressBarRequested);
    connect(&reloader, &RecommendationsProcessor::updateCounter, this,      &ServitorWindow::OnUpdatedProgressValue);
    connect(&reloader, &RecommendationsProcessor::updateInfo, this,         &ServitorWindow::OnNewProgressString);


    reloader.SetStagedAuthors(authors);
    reloader.ReloadRecommendationsList(fetching::CacheStrategy::CacheOnly());
    authorInterface->AssignAuthorNamesForWebIDsInFanficTable();
}

void ServitorWindow::on_pbGetNewFavourites_clicked()
{
    if(!env.ResumeUnfinishedTasks()){
        fetching::CacheStrategy customCacheStrategy;
        customCacheStrategy.useCache = true;
        customCacheStrategy.cacheExpirationDays = 7;
        customCacheStrategy.fetchIfCacheIsOld = true;
        customCacheStrategy.fetchIfCacheUnavailable = true;
        env.LoadAllLinkedAuthors(customCacheStrategy);
    }
}

void ServitorWindow::on_pbReprocessCacheLinked_clicked()
{
    env.LoadAllLinkedAuthorsMultiFromCache();
}

void ServitorWindow::on_pbSlashCalc_clicked()
{
    auto db = sql::Database::database();
    sql::Transaction transaction(db);
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;


    auto fandomInterface = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    fandomInterface->db = db;

    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanficsInterface->db = db;
    fanficsInterface->authorInterface = authorInterface;
    fanficsInterface->fandomInterface = fandomInterface;

    auto recsInterface = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    recsInterface->db = db;
    recsInterface->authorInterface = authorInterface;

    SlashProcessor slash(db,fanficsInterface, fandomInterface, authorInterface, recsInterface);
    slash.DoFullCycle(db, 2);
    transaction.finalize();
    QLOG_INFO() << "finished";

}

void ServitorWindow::on_pbFindSlashSummary_clicked()
{
    CommonRegex rx;
    rx.Init();
    auto result = rx.ContainsSlash("(feminine!Ulqi was the 'forgotten child'"
                                   " of his family, thanks KaiShin to his prodigy like elder siblings and his little sisters' status as 'The Girl Who lived'."
                                   " However, he was not going to let this stop him from becoming the best. With an indomitable will and the resolve to do"
                                   " whatever he must, he will either ascended to the top, or die trying.",
                                   "[Ichigo K., Yoruichi S.] Rukia K., T. Harribel",
                                   "Detective Conan/Case Closed");
    Q_UNUSED(result);
}

void ServitorWindow::LoadDataForCalculation(CalcDataHolder& cdh)
{
    auto db = sql::Database::database();
    auto genresInterface  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanficsInterface = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authorsInterface= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    genresInterface->db = db;
    fanficsInterface->db = db;
    authorsInterface->db = db;

    database::Transaction transaction(db);


    if(!QFile::exists("TempData/fandomstats_0.txt"))
    {
        TimedAction action("Loading data",[&](){
            cdh.fics = env.interfaces.fanfics->GetAllFicsWithEnoughFavesForWeights(ui->sbFicCount->value());
            cdh.favourites = authorsInterface->LoadFullFavouritesHashset();
            cdh.genreData = authorsInterface->GetListGenreData();
            qDebug() << "got genre lists, size: " << cdh.genreData.size();
            cdh.fandomLists = authorsInterface->GetAuthorListFandomStatistics(cdh.favourites.keys());
            qDebug() << "got fandom lists, size: " << cdh.fandomLists.size();
            cdh.authors = authorsInterface->GetAllAuthors("ffn");
        });
        action.run();

        TimedAction saveAction("Saving data",[&](){
            cdh.SaveFicsData();
        });
        saveAction.run();
    }
    else
    {
        TimedAction loadAction("Loading data",[&](){
            cdh.LoadFicsData();
        });
        loadAction.run();
    }
    transaction.finalize();
    QSet<int> ficSet;
    for(const auto& fic : std::as_const(cdh.fics))
        ficSet.insert(fic->id);

    qDebug() << "ficset is of size:" << ficSet.size();

    // need to reduce fav sets and remove every fic that isn't in the calculation
    // to reduce throttling
    // perhaps doesn't need to be a set, a vector might do
    for(auto& favSet : cdh.favourites)
    {
        int previousSize = favSet.size();
        favSet.intersect(ficSet);
        int newSize = favSet.size();
        qDebug() << "P: " << previousSize << "N: " << newSize;
    }

    auto it = cdh.favourites.begin();
    auto end = cdh.favourites.end();
    while(it != end)
    {
        if(it.value().size() < 1200)
            cdh.filteredFavourites.push_back({it.key(), it.value()});
        it++;
    }

    std::sort(cdh.filteredFavourites.begin(), cdh.filteredFavourites.end(), [](const ListWithIdentifier& fp1, const ListWithIdentifier& fp2){
        return fp1.favourites.size() < fp2.favourites.size();
    });
    for(auto& list: cdh.filteredFavourites)
        qDebug() << "Size: " << list.favourites.size();

}

struct FicPair{
    //    /uint32_t count = 0;
    //    float val1;
    //    float val2;
    //    float val3;
    //    float val4;
    //    float val5;
    Roaring r;
};

//struct SmartHash{
//    void CleanTemporaryStorage();
//    void PrepareForList(const QList<QPair<uint32_t, uint32_t>>& list);

//    QHash<QPair<uint32_t, uint32_t>, Roaring> workingSet;
//};


void ServitorWindow::ProcessCDHData(CalcDataHolder& cdh){

    for(const auto& fav : std::as_const(cdh.filteredFavourites))
    {
        for(auto fic : fav.favourites)
        {
            ficsToFavLists[fic].setCopyOnWrite(true);
            ficsToFavLists[fic].add(fav.id);
        }
    }

    for(const auto& fic : std::as_const(cdh.fics))
    {
        if(!ficsToFavLists.contains(fic->id))
            continue;
        ficData[fic->id] = fic;
        for(const auto& fandom : std::as_const(fic->fandoms))
            ficsForFandoms[fandom].insert(fic->id);
    }
    qDebug() << "Will work with: " << ficData.size() << " fics";
    qDebug() << 1;

    qDebug() << 2;
}


void ServitorWindow::on_pbCalcWeights_clicked()
{
    CalcDataHolder cdh;
    LoadDataForCalculation(cdh);
    ProcessCDHData(cdh);
    //QHash<int, core::FicWeightResult> result;
    //QSet<int> alreadyProcessed;

    //QHash<QPair<int, int>, QSet<int>> meetingSet;
    QHash<QPair<uint32_t, uint32_t>, FicPair> ficsMeeting;
    QHash<QPair<uint32_t, uint32_t>, FicPair>::iterator ficsIterator;
    QHash<QPair<int, int>, QSet<int>>::iterator meetingIterator;
    //QHash<QPair<uint32_t, uint32_t>, Roaring> roaringSet;
    QHash<QPair<uint32_t, uint32_t>, Roaring>::iterator roaringIterator;


    //            {
    //                auto ficIds = ficData.keys();
    //                for(int i = 0; i < ficIds.size(); i++)
    //                {
    //                    if(i%10 == 0)
    //                        qDebug() << "working at: " << i;
    //                    auto fic1 = ficIds[i];
    //                    for(int j = i+1; j < ficIds.size(); j++)
    //                    {
    //                        auto fic2 = ficIds[i];
    //                        auto keys = cdh.favourites.keys();
    //                        QVector<int> intersection;
    //                        auto& set1 = ficsToFavLists[fic1];
    //                        auto& set2 = ficsToFavLists[fic2];
    //                        intersection.reserve(std::max(set1.size(), set2.size()));
    //                        std::set_intersection(set1.begin(), set1.end(),
    //                                              set2.begin(), set2.end(),
    //                                               std::back_inserter(intersection));
    //                        ficsMeeting[{fic1, fic2}].count = intersection.size();
    //                    }
    //                }
    //            }
    // need to create limited sets of unique pairs
    //    QList<QPair<int, int>> pairList;
    //    {
    //        auto ficIds = ficData.keys();
    //        for(int i = 0; i < ficIds.size(); i++)
    //        {
    //            for(int j = i+1; j < ficIds.size(); j++)
    //            {
    //                pairList.push_back({ficIds[i], ficIds[j]});
    //            }
    //        }
    //    }

    {
        int counter = 0;
        QPair<uint32_t, uint32_t> pair;
        qDebug() << "full fav size: " << cdh.filteredFavourites.size();
        for(const auto& fav : std::as_const(cdh.filteredFavourites))
        {
            auto values = fav.favourites.toList();
            if(counter%100 == 0)
                qDebug() << " At: " << counter << " size: " << values.size();
            counter++;
            if(counter > 213000 && counter%1000 == 0)
                ficsMeeting.squeeze();

            //qDebug() << "Reading list of size: " << values.size();
            for(int i = 0; i < values.size(); i++)
            {
                for(int j = i+1; j < values.size(); j++)
                {
                    uint32_t fic1 = values[i];
                    uint32_t fic2 = values[j];

                    //QPair<uint32_t, uint32_t> pair (std::min(fic1,fic2), std::max(fic1,fic2));
                    pair.first = std::min(fic1,fic2);
                    pair.second = std::max(fic1,fic2);
                    //QPair<uint32_t, uint32_t> pair2 = {std::min(fic1,fic2), std::max(fic1,fic2)};
                    //qDebug() << pair;
                    //qDebug() << pair2;
                    //roaring attempt
                    //                    roaringIterator = roaringSet.find(pair);
                    //                    if(roaringIterator == roaringSet.end())
                    //                    {
                    //                        roaringSet[pair];
                    //                        roaringIterator = roaringSet.find(pair);
                    //                    }

                    //                    roaringIterator->add(fav.id);
                    //                    roaringIterator->shrinkToFit();
                    //meeting attempt
                    //                    meetingIterator = meetingSet.find(pair);
                    //                    if(meetingIterator == meetingSet.end())
                    //                    {
                    //                        meetingSet[pair];
                    //                        meetingIterator = meetingSet.find(pair);
                    //                    }

                    //                    meetingIterator->insert(key);
                    //valueattempt
                    ficsIterator = ficsMeeting.find(pair);
                    if(ficsIterator == ficsMeeting.end())
                    {
                        ficsMeeting[pair];
                        ficsIterator = ficsMeeting.find(pair);
                    }

                    //ficsIterator.value().count++;
                    ficsIterator.value().r.add(fav.id);
                    if(counter > 213000)
                        ficsIterator.value().r.shrinkToFit();
                    //                    pairCounter++;
                }
            }
            //qDebug() << "List had: " << pairCounter << " pairs";
        }
    }
    int countPairs = 0;
    int countListRecords = 0;

    for(const auto& edge: std::as_const(ficsMeeting))
    {
        countPairs++;
        countListRecords+=edge.r.cardinality();
    }
    qDebug() << "edge count: " << countPairs;
    qDebug() << "list records count: " << countListRecords;
    //    auto values = ficsMeeting.values();
    //    std::sort(values.begin(), values.end(), [](const FicPair& fp1, const FicPair& fp2){
    //        return fp1.count > fp2.count;
    //    });
    //    QString prototype = "SELECT * FROM fanfics where  id in (%1, %2)";
    //    for(int i = 0; i < 10; i++)
    //    {
    //        QString temp = prototype;
    //        //qDebug() << << values[i].fic1 << " " << values[i].fic2 << " " << values[i].meetings.size();
    //        qDebug().noquote() << temp.arg(values[i].fic1) .arg(values[i].fic2);
    //        qDebug() << "Met times: " << values[i].count;
    //        qDebug() << " ";
    //    }

    //fanficsInterface->WriteFicRelations(result);
}


void ServitorWindow::on_pbCleanPrecalc_clicked()
{
    QFile::remove("ficsdata.txt");
}


void ServitorWindow::on_cbGenres_currentIndexChanged(const QString &arg1)
{
    InitGenreChartView(arg1);
}

void ServitorWindow::on_cbMoodSelector_currentTextChanged(const QString &arg1)
{
    InitMoodChartView(arg1);
}

void ServitorWindow::on_pbCalcFicGenres_clicked()
{
    CreateSecondIterationOfGenresForFics(25, 15);
}

void ServitorWindow::on_pbCompareGenres_clicked()
{
    QLOG_INFO() << "on_pbCompareGenres_clicked 1";
    if(ui->leGenreUser1->text().isEmpty() || ui->leGenreUser2->text().isEmpty())
        return;

    int user1 = ui->leGenreUser1->text().trimmed().toInt();
    QString otherUsers = ui->leGenreUser2->text().trimmed();
    QLOG_INFO() << "on_pbCompareGenres_clicked 1.1";


    auto db = sql::Database::database();
    QLOG_INFO() << "on_pbCompareGenres_clicked 1.2";
    auto genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    auto authors= QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    QLOG_INFO() << "on_pbCompareGenres_clicked 1.3";
    genres->db = db;
    fanfics->db = db;
    authors->db = db;
    QLOG_INFO() << "on_pbCompareGenres_clicked 1.4";


    QList<int> users;
    QLOG_INFO() << "on_pbCompareGenres_clicked 1.5";
    users << user1;
    ui->cbUserIDs->blockSignals(true);
    ui->cbUserIDs->clear();
    ui->cbUserIDs->blockSignals(false);
    QLOG_INFO() << "on_pbCompareGenres_clicked 1.6";
    InitGrpcSource();
    QLOG_INFO() << "on_pbCompareGenres_clicked 2";
    QList<int> otherUserIds;
    const auto tempList = otherUsers.split(QRegExp("[\\s,]"), Qt::SkipEmptyParts);
    for(const auto& user: tempList)
    {
        users.push_back(user.toInt());
        otherUserIds.push_back(user.toInt());
        ui->cbUserIDs->blockSignals(true);
        ui->cbUserIDs->addItem(user);
        ui->cbUserIDs->blockSignals(false);
    }
    QStringList ficList, ignoreList;
    if(ui->edtLog->toPlainText().isEmpty())
        matchesForUsers = grpcSource->GetMatchesForUsers(user1, otherUserIds);
    else
    {
        ficList = ui->edtLog->toPlainText().split(QRegExp("[\\s,]"), QString::SkipEmptyParts);
        ignoreList = ui->edtIgnores->toPlainText().split(QRegExp("[\\s,]"), QString::SkipEmptyParts);
        matchesForUsers = grpcSource->GetMatchesForUsers({ficList, ignoreList}, otherUserIds);
    }
    QLOG_INFO() << "on_pbCompareGenres_clicked 3";


    if(!ui->chkOver50->isChecked())
    {
        QList<int> toErase;

        for(auto i = matchesForUsers.begin(); i != matchesForUsers.end(); i++)
        {
            if(i.value().ratioWithoutIgnores > 50)
                toErase.push_back(i.key());
        }
        for(auto user : toErase)
        {
            matchesForUsers.remove(user);
        }
        users = {user1};

        for(auto i = matchesForUsers.begin(); i != matchesForUsers.end(); i++)
            users.push_back(i.key());

    }
    QLOG_INFO() << "on_pbCompareGenres_clicked 4";
    if(authorGenreDataOriginal.size() == 0)
    {

        LoadDataForCalc<core::rdt_favourites,
                InputForGenresIteration2::FavType,
                QSharedPointer<interfaces::Authors>>(authors, inputs.faves, "TempData");

        auto& faves  = inputs.faves;
        LoadDataForCalc<core::rdt_author_genre_distribution,
                QHash<int, std::array<double, 22>>,
                QSharedPointer<interfaces::Authors>>(authors, authorGenreDataOriginal, "TempData", "original");
        LoadDataForCalc<core::rdt_fic_genres_composite,
                InputForGenresIteration2::FicGenreCompositeType,
                QSharedPointer<interfaces::Genres>>(genres, inputs.ficGenresComposite, "TempData");

        genres->loadOriginalGenresOnly = true;
        LoadDataForCalc<core::rdt_fic_genres_composite,
                InputForGenresIteration2::FicGenreCompositeType,
                QSharedPointer<interfaces::Genres>>(genres, inputs.ficGenresOriginalsInCompositeFormat, "TempData", "CF");
        genres->loadOriginalGenresOnly = false;

//        LoadDataForCalc<core::rdt_fic_genres_original,
//                InputForGenresIteration2::FicGenreOriginalType,
//                QSharedPointer<interfaces::Genres>>(genres, inputs.ficGenresOriginal, "TempData");
        QLOG_INFO() << "on_pbCompareGenres_clicked 5";
        originalMoodData = iteratorProcessor.CreateMoodDataFromGenres(authorGenreDataOriginal);
        iteratorProcessor.ReprocessGenreStats(inputs.ficGenresComposite, faves);
        adjustedMoodData = iteratorProcessor.resultingMoodAuthorData;
        QLOG_INFO() << "on_pbCompareGenres_clicked 6";
        {
            AuthorGenreIterationProcessor iteratorProcessor;
            Roaring r;
            for(const auto& fic : std::as_const(ficList))
                r.add(fic.toInt());

            QHash<int, Roaring> favourites;
            favourites[ui->leGenreUser1->text().toInt()] = r;
            iteratorProcessor.ReprocessGenreStats(inputs.ficGenresOriginalsInCompositeFormat, favourites);
            listGenreData = iteratorProcessor.resultingGenreAuthorData[ui->leGenreUser1->text().toInt()];
            listMoodData = iteratorProcessor.resultingMoodAuthorData[ui->leGenreUser1->text().toInt()];
            iteratorProcessor.ReprocessGenreStats(inputs.ficGenresComposite, favourites);
            adjustedListGenreData = iteratorProcessor.resultingGenreAuthorData[ui->leGenreUser1->text().toInt()];
            adjustedListMoodData = iteratorProcessor.resultingMoodAuthorData[ui->leGenreUser1->text().toInt()];
        }
    }
    QLOG_INFO() << "on_pbCompareGenres_clicked 7";



    FillFicsForUser(ui->cbUserIDs->currentText());
    QLOG_INFO() << "on_pbCompareGenres_clicked 8";

    bool useOriginalMood =ui->cbMoodSource->currentIndex() == 1;
    if(ui->cbGenreMood->currentIndex() == 0)
        InitMoodCompareChartView(users, useOriginalMood);
    else
        InitGenreCompareChartView(users);

}

void ServitorWindow::FillFicsForUser(QString user)
{
    InitGrpcSource();
    auto fullMatchList = CreateSummaryMatches();
    QHash<int, core::Fanfic> fics;
    for(auto i = fullMatchList.begin(); i != fullMatchList.end(); i++)
    {
        QVector<core::Fanfic> tempFics;
        grpcSource->FetchFic(i.key(), &tempFics, core::StoryFilter::EUseThisFicType::utf_db_id);
        fics[i.key()]=tempFics[0];
    }
    ui->edtFics->clear();
    ui->edtGenreBreakdown->clear();

    auto keys = fics.keys();
    std::sort(keys.begin(), keys.end(), [&](int k1, int k2){
        if(fullMatchList[k1] > fullMatchList[k2])
            return true;
        if(fullMatchList[k1] < fullMatchList[k2])
            return false;
        return  fics[k1].title < fics[k2].title;
    });

    QHash<QString, double> genreBreakdown;
    QHash<QString, double> combinedGenreBreakdown;

    for(auto fic : keys)
    {
        QString matches = QString::number(fullMatchList[fic]);
        QStringList genreList;
        for(const auto& genre: std::as_const(inputs.ficGenresComposite[fic]))
        {
            for(const auto& genreBit: std::as_const(genre.genres))
            {
                if(matchesForUsers[user.toInt()].matches.contains(fic))
                    genreBreakdown[genreBit] += genre.relevance;
                combinedGenreBreakdown[genreBit] += genre.relevance;
                genreList.push_back(genreBit);
            }
        }
        //QString genresJoined = genreList.join("/");
        QString str = QString("(%1)").arg(matches) + " " /*+ genresJoined.leftJustified(40, ' ') + " " */ + fics[fic].title;
        str = str.replace(" ", "&nbsp;");
        if(matchesForUsers[user.toInt()].matches.contains(fic))
            ui->edtFics->append(QString("<html><b>%1</b</html>").arg(str));
        else
            ui->edtFics->append(QString("<html>%1</html>").arg(str));
    }
    genre_stats::ListMoodData relativeMoodData;
    genre_stats::ListMoodData relativeCombinedMoodData;
    {
    auto genreKeys = genreBreakdown.keys();
    std::sort(genreKeys.begin(), genreKeys.end(), [&](QString k1, QString k2){
        return  genreBreakdown[k1] > genreBreakdown[k2];
    });
    ui->edtGenreBreakdown->append(QString("<html>Size: %1</html>").arg(matchesForUsers[user.toInt()].matches.size()));

    ui->edtGenreBreakdown->setVisible(false);
    for(const auto& key : genreKeys)
    {
        //QString str = key + " " + QString::number(genreBreakdown[key]);
        //ui->edtGenreBreakdown->append(QString("<html>%1</html>").arg(str));
        QString mood = interfaces::Genres::MoodForGenre(key);
        interfaces::Genres::WriteMoodValue(mood,
                                           genreBreakdown[key],
                                           relativeMoodData);

    }
    }
    {
        auto genreKeys = combinedGenreBreakdown.keys();
        std::sort(genreKeys.begin(), genreKeys.end(), [&](QString k1, QString k2){
            return  combinedGenreBreakdown[k1] > combinedGenreBreakdown[k2];
        });
        ui->edtGenreBreakdown->setVisible(false);
        for(const auto& key : genreKeys)
        {
            //QString str = key + " " + QString::number(genreBreakdown[key]);
            //ui->edtGenreBreakdown->append(QString("<html>%1</html>").arg(str));
            QString mood = interfaces::Genres::MoodForGenre(key);
            interfaces::Genres::WriteMoodValue(mood,
                                               combinedGenreBreakdown[key],
                                               relativeCombinedMoodData);

        }
    }
    relativeMoodData.DivideByCount(matchesForUsers[user.toInt()].matches.size());
    relativeCombinedMoodData.DivideByCount(fullMatchList.size());
    InitMatchingListChartView(relativeMoodData, relativeCombinedMoodData);

}

QHash<int, int> ServitorWindow::CreateSummaryMatches()
{
    QHash<int, int> result;

    for(auto i = matchesForUsers.begin(); i != matchesForUsers.end(); i++)
    {
        for(auto fic: std::as_const(i.value().matches))
        {
            result[fic]++;
        }
    }
    return result;
}
void ServitorWindow::on_cbMoodSource_currentIndexChanged(const QString &)
{
    on_pbCompareGenres_clicked();
}

void ServitorWindow::on_cbUserIDs_currentIndexChanged(const QString &)
{
    FillFicsForUser(ui->cbUserIDs->currentText());
}

//void ServitorWindow::on_pbExtractDiscords_clicked()
//{
//    QString path = "PageCache.sqlite";
//    sql::Database pcdb = sql::Database::addDatabase("QSQLITE", "PageCache");
//    pcdb.setDatabaseName(path);
//    pcdb.open();

//    sql::Query testQuery("select * from pagecache order by url desc", pcdb);
//    ui->tbrDiscords->setOpenExternalLinks(true);
//    int i = 0;
//    while(testQuery.next())
//    {
//        i++;
//        auto temp = QString::fromUtf8(qUncompress(testQuery.value("CONTENT").toByteArray()));
//        if(temp.contains(QRegExp("5px.*(Discord).*(?=#xtab)")))
//        {
//            auto url = testQuery.value("URL").toString();
//            ui->tbrDiscords->insertHtml("<a href=\"" + url + "\">" + url + " </a>");
//            ui->tbrDiscords->insertHtml("<br>");
//            qDebug() << url;
//        }
//        QApplication::processEvents();
//        if(i%100==0)
//            ui->tbrDiscords->insertHtml("<br>i is at: " + QString::number(i) + "<br>");
//        if(stopDiscordProcessing)
//            break;
//    }

//}

//void ServitorWindow::on_pbExtractDiscords_clicked()
//{
//    QString path = "PageCache.sqlite";
//    sql::Database pcdb = sql::Database::addDatabase("QSQLITE", "PageCache");
//    pcdb.setDatabaseName(path);
//    pcdb.open();

//    sql::Query testQuery("select * from pagecache order by url desc", pcdb);
//    ui->tbrDiscords->setOpenExternalLinks(true);
//    int i = 0;
//    while(testQuery.next())
//    {
//        i++;
//        auto temp = QString::fromUtf8(qUncompress(testQuery.value("CONTENT").toByteArray()));
//        QRegExp rxCapital("Discord");
//        QRegExp rxSmol("discord");
//        QRegExp rxPony("My\\sLittle\\sPony");

//        QRegExp begin ("Send\\sPrivate\\sMessage");
//        auto indexBegin = begin.indexIn(temp);

//        QRegExp end ("xtab");
//        auto indexEnd= end.indexIn(temp, indexBegin);
//        if(indexBegin != -1 && indexEnd != -1){

//            QString bio = temp.mid(indexBegin, temp.length() - indexBegin - (temp.length() - indexEnd));
//            //qDebug() << bio;
//            auto indexCapital = rxCapital.indexIn(bio);
//            auto indexSmol = rxSmol.indexIn(bio);
//            auto hasPony= temp.contains("My Little Pony");
//            if(indexCapital !=-1 && indexSmol == -1 && !hasPony)
//            {
//                auto url = testQuery.value("URL").toString();
//                qDebug() << url;
//            }
//        }

//        QApplication::processEvents();
//        if(i%100==0)
//            ui->tbrDiscords->insertHtml("<br>i is at: " + QString::number(i) + "<br>");
//        if(stopDiscordProcessing)
//            break;
//    }

//}


void ServitorWindow::on_pbExtractDiscords_clicked()
{
    QString path = "PageCache.sqlite";
    auto pcDb = database::sqlite::InitAndUpdateSqliteDatabaseForFile("database","PageCache","dbcode/pagecacheinit.sql", "PageCache", false);

    sql::Query testQuery("select * from pagecache order by url desc", pcDb);
    ui->tbrDiscords->setOpenExternalLinks(true);
    int i = 0;
    while(testQuery.next())
    {
        i++;
        auto temp = QString::fromUtf8(qUncompress(testQuery.value("CONTENT").toByteArray()));
        QRegularExpression rxCapital("[A-Za-z0-9]{6,7}(\\s|$|[.])");
        QRegExp rxSmol("(discord|Discord)");
        //QRegExp rxPony("My\\sLittle\\sPony");

        QRegExp begin ("Send\\sPrivate\\sMessage");
        auto indexBegin = begin.indexIn(temp);

        QRegExp end ("xtab");
        auto indexEnd= end.indexIn(temp, indexBegin);
        if(indexBegin != -1 && indexEnd != -1){

            QString bio = temp.mid(indexBegin, temp.length() - indexBegin - (temp.length() - indexEnd));
            //qDebug() << bio;
            auto matchCapital = rxCapital.match(bio);
            auto indexSmol = rxSmol.indexIn(bio);
            Q_UNUSED(indexSmol);
            auto hasPony= temp.contains("My Little Pony");
            Q_UNUSED(hasPony);
            if(matchCapital.hasMatch() /*&& indexSmol == -1 && !hasPony*/)
            {
                    const auto list = matchCapital.capturedTexts();
                    for(const auto& text: list)
                    {
                        if(!text.contains("Private") && !text.trimmed().isEmpty())
                        {
                            qDebug() << text;
                        }
                    }

                //qDebug() << "Entering match";
//                qDebug() << "Caplength: " << matchCapital.capturedTexts().length();
//                qDebug() << "Captext: " << matchCapital.capturedTexts();
                bool found = false;
                const auto tempList = matchCapital.capturedTexts();
                for(const auto& text: tempList)
                {

                    QRegularExpression rxCapitalSearch("[A-Z0-9]");
                    auto match = rxCapitalSearch.match(text);
                    if(text.contains("Private") || text.trimmed().isEmpty())
                        continue;
                    qDebug() << match.capturedTexts();
                    if(match.capturedTexts().length() >= 2)
                    {
                        found = true;
                    }
                    if(found)
                    {
                        auto url = testQuery.value("URL").toString();
                        qDebug() << match.capturedTexts() << " " << QString::fromStdString(url);
                    }
                }
            }
        }

        QApplication::processEvents();
        if(i%100==0)
            ui->tbrDiscords->insertHtml("<br>i is at: " + QString::number(i) + "<br>");
        if(stopDiscordProcessing)
            break;
    }

}

void ServitorWindow::on_pbStopDiscordProcessing_clicked()
{
    stopDiscordProcessing = true;
}

void ServitorWindow::on_pbTestDiscord_clicked()
{
    if(ui->tbrDiscords->toPlainText().contains(QRegExp(ui->leDiscordRegex->text())))
        qDebug() << "valid";
    else {
        qDebug() << "invalid";
    }
}

