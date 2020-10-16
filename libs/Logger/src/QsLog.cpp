// Copyright (c) 2013, Razvan Petru
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "QsLog.h"
#include "QsLogDest.h"
#ifdef QS_LOG_SEPARATE_THREAD
#include <QThreadPool>
#include <QRunnable>
#else
#include <QMutex>
#endif
#include <QVector>
#include <QDateTime>
#include <QtGlobal>
#include <QReadWriteLock>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include "Tracer.h"


namespace QsLogging
{

// not using Qt::ISODate because we need the milliseconds too
//static const QString fmtDateTime("dd hh:mm:ss.zzz");

static inline QString LevelToText(Level theLevel)
{
    switch (theLevel) {
        case TraceLevel:
            return "TRACE";
        case DebugLevel:
            return "DEBUG";
        case InfoLevel:
            return "INFO";
        case WarnLevel:
            return "WARN";
        case ErrorLevel:
            return "ERROR";
        case FatalLevel:
            return "FATAL";
        case OffLevel:
            return "";
        default: {
            assert(!"bad log level");
            return "INFO";
        }
    }
}

#ifdef QS_LOG_SEPARATE_THREAD
class LogWriterRunnable : public QRunnable
{
public:
    LogWriterRunnable(const QString &message, Level level)
        : mMessage(message)
        , mLevel(level) {}

    virtual void run()
    {
        Logger::instance().write(mMessage, mLevel);
    }

private:
    QString mMessage;
    Level mLevel;
};
#endif

class LoggerImpl
{
public:
    LoggerImpl() :
        logMutex(QReadWriteLock::Recursive),level(InfoLevel)
    {
        // assume at least file + console
        destList.reserve(2);
#ifdef QS_LOG_SEPARATE_THREAD
        threadPool.setMaxThreadCount(1);
        threadPool.setExpiryTimeout(-1);
#endif
    }
#ifdef QS_LOG_SEPARATE_THREAD
    QThreadPool threadPool;
#else
    QReadWriteLock logMutex;
#endif
    Level level;
    DestinationList destList;
};

Logger::Logger() :
    d(new LoggerImpl)
{
}

Logger::~Logger()
{
    delete d;
}

void Logger::addDestination(DestinationPtr destination)
{
    assert(destination.data());
    QWriteLocker lock(&d->logMutex);
    d->destList.push_back(destination);
}

void Logger::replaceDestination(DestinationPtr destination)
{
    assert(destination.data());
    QWriteLocker lock(&d->logMutex);
    d->destList.clear();
    d->destList.push_back(destination);
}

void Logger::clearDestinationList()
{
    QWriteLocker lock(&d->logMutex);
    d->destList.clear();
}

void Logger::setLoggingLevel(Level newLevel)
{
    QWriteLocker lock(&d->logMutex);
    d->level = newLevel;
}

Level Logger::loggingLevel() const
{
    QReadLocker lock(&d->logMutex);
    return d->level;
}

void Logger::clearDestinationQueues()
{
    QWriteLocker lock(&d->logMutex);
    for(DestinationPtr dest : qAsConst(d->destList))
    {
        dest->clearQueue();
    }
}

DestinationList Logger::GetDestinations()
{
    QReadLocker lock(&d->logMutex);
    return d->destList;
}

void Logger::ResetDestinations()
{
    QWriteLocker lock(&d->logMutex);
    d->destList.clear();
}

//! creates the complete log message and passes it to the logger
void Logger::Helper::writeToLog()
{
    if(writeServiceInfo)
        loggerInstance->enqueueWrite(LevelToText(level) + " " + QDateTime::currentDateTimeUtc().toString(QStringLiteral("dd hh:mm:ss.zzz")) + " " + buffer, level);
    else
        loggerInstance->enqueueWrite(buffer, level);
}

Logger::Helper::Helper(Level logLevel, Logger *_loggerInstance, bool _writeServiceInfo) :
    level(logLevel),
    qtDebug(&buffer), loggerInstance(_loggerInstance), writeServiceInfo(_writeServiceInfo)
{
    assert(loggerInstance);
#if QT_VERSION == QT_VERSION_CHECK(5, 5, 0)
    qtDebug = qtDebug.noquote();
#endif
}


Logger::Helper::~Helper()
{
    try {
        writeToLog();
    }
    catch(std::exception&) {
        // you shouldn't throw exceptions from a sink
        assert(!"exception in logger helper destructor");
        //throw;
    }
}

//! directs the message to the task queue or writes it directly
void Logger::enqueueWrite(const QString& message, Level level)
{
#ifdef QS_LOG_SEPARATE_THREAD
    LogWriterRunnable *r = new LogWriterRunnable(message, level);
    d->threadPool.start(r);
#else

    write(message, level);
#endif
}

//! Sends the message to all the destinations. The level for this message is passed in case
//! it's useful for processing in the destination.
void Logger::write(const QString& message, Level level)
{
    QWriteLocker lock(&d->logMutex);
    for (auto it = d->destList.begin(), endIt = d->destList.end(); it != endIt;++it)
    {
            (*it)->write(message, level, d->level);
    }
}

} // end namespace
