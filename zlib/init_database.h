#pragma once
#include <QString>
namespace database{
    bool ReadDbFile();
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool crossover, bool);
    bool FetchTrackStateForFandom(QString fandom, bool crossover);
    void PushFandom(QString);
    void RebaseFandoms();
    QStringList FetchRecentFandoms();
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();
    void BackupDatabase();

}
