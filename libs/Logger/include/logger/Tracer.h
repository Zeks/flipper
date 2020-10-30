#pragma once
#include <sstream>

#include <QSettings>
#include <QString>
#include <QTextCodec>
#include <QThreadStorage>

#include <QElapsedTimer>

//#include "GlobalHeaders/c11logger.h"
#include "GlobalHeaders/run_once.h"
#include "QsLogger.h"


class FunctionTracer
{
public:
    FunctionTracer(QString val):traceString(val)
    {
        static bool traceEnabled = []{
            QSettings settings(QStringLiteral("Settings/local.ini"), QSettings::IniFormat);
            settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
            return settings.value(QStringLiteral("Main/traceFunctions")).toBool();
        }();
        if(!traceEnabled)
            return;
        FuncCleanup a([&](){logLevel.setLocalData(logLevel.localData()+1);});
        Q_UNUSED(a);
        startString = QString();
        startString += {QString("|").repeated(logLevel.localData()) + QStringLiteral("Entered function ") + traceString};
        QLOG_TRACE() << startString;

    }
    FunctionTracer(const FunctionTracer&) = default;
    ~FunctionTracer()
    {
        static bool traceEnabled = []{
            QSettings settings(QStringLiteral("Settings/local.ini"), QSettings::IniFormat);
            settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
            return settings.value(QStringLiteral("Main/traceFunctions")).toBool();
        }();
        if(!traceEnabled)
            return;
        QLOG_TRACE() << QString::number(timer.elapsed()) + QStringLiteral(" ") + QString(QStringLiteral("|")).repeated(logLevel.localData()) + QStringLiteral("Left    function ") + traceString;
        FuncCleanup a([&](){logLevel.setLocalData(logLevel.localData()-1);});
        Q_UNUSED(a);

        QString endString;
        endString+=  QString(QStringLiteral("|")).repeated(logLevel.localData()) +
                QStringLiteral("Left    function ") +
                traceString;
        QLOG_TRACE() << endString;
    }

    QString traceString;
    QString startString;
    QElapsedTimer timer;
    static QThreadStorage<int> logLevel;

};
//#undef GetMessage
//#undef AddJob
//#undef interface
#define TRACE_FUNCTION FunctionTracer function_tracer(Q_FUNC_INFO);
#define TRACE_GENERATOR_FUNCTION
#define TRACE_FIELD_FUNCTION



