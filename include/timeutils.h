#pragma once

#include <functional>
#include <chrono>
#include <QString>
#include <QDebug>

struct TimedAction{
    TimedAction(QString name, std::function<void()> action)
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
            auto ms = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            qDebug() << "Action: " << actionName << " Performed in: " << ms;
        }
        else
            action();

    }
    std::function<void()> action;
    QString actionName;
};
