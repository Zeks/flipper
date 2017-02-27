#pragma once
#include <QString>
namespace database{
    bool ReadDbFile();
    bool ReindexTable(QString table);
    void SetFandomTracked(QString section, QString fandom, bool crossover, bool);
    void PushFandom(QString);
    void RebaseFandoms();
    QStringList FetchRecentFandoms();

    bool FetchTrackStateForFandom(QString section, QString fandom, bool crossover);
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();
    void BackupDatabase();

}
