#pragma once
#include <QString>
namespace database{
    bool ReadDbFile();
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool crossover, bool);
}
