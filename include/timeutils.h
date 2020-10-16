/*Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once

#include <functional>
#include <chrono>
#include <QString>
#include <QDebug>
#include "logger/QsLog.h"


struct TimedAction{
    TimedAction(QString name, const std::function<void()>& action)
    {
        this->actionName = name;this->action = action;
    }
    void run(bool log = true)
    {
        if(log)
        {
            auto start = std::chrono::high_resolution_clock::now();
            action();
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            ms = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            QLOG_INFO() << "Action: " << actionName << " Performed in: " << ms;
        }
        else
            action();

    }
    std::function<void()> action;
    QString actionName;
    long long ms;

};

struct TimeKeeper{
    TimeKeeper(){
        times[QStringLiteral("start")] = std::chrono::high_resolution_clock::now();
    }
    void Log(QString value, QString startPoint,  bool inSeconds = false){
        times[value] = std::chrono::high_resolution_clock::now();
        if(!inSeconds)
            QLOG_INFO() << "Time spent in " << value << " "
                     << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - times[startPoint]).count();
        else
            QLOG_INFO() << "Time spent in " << value << " "
                     << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - times[startPoint]).count();
    }
    void Track(QString value){times[value] = std::chrono::high_resolution_clock::now();}
    QHash<QString, std::chrono::high_resolution_clock::time_point> times;
};
