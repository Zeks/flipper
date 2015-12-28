
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

#ifndef QSLOG_H
#define QSLOG_H
#include <sstream>
#include <thread>
#include "QsLogLevel.h"
#include "QsLogDest.h"
#include <QDebug>
#include <QString>
#include "GlobalHeaders/SingletonHolder.h"
#include "l_logger_global.h"
#define QS_LOG_VERSION "2.0b1"

namespace QsLogging
{
class Destination;
typedef QVector<DestinationPtr> DestinationList;
class LoggerImpl; // d pointer



class L_LOGGERSHARED_EXPORT Logger
{
public:

    //! Adds a log message destination. Don't add null destinations.
    void addDestination(DestinationPtr destination);
    void replaceDestination(DestinationPtr destination);
    //! Logging at a level < 'newLevel' will be ignored
    void setLoggingLevel(Level newLevel);
    //! The default level is INFO
    Level loggingLevel() const;
    void clearDestinationQueues();
    DestinationList GetDestinations();
    //! The helper forwards the streaming to QDebug and builds the final
    //! log message.
    Logger();
    class L_LOGGERSHARED_EXPORT Helper
    {
    public:
        explicit  Helper(Level logLevel, Logger* _loggerInstance, bool _writeServiceInfo = true);
        ~Helper();
        QDebug& stream()
        {
            return qtDebug;
        }

    private:
        void writeToLog();

        Level level;
        QString buffer;
        QDebug qtDebug;
        Logger* loggerInstance;
        bool writeServiceInfo = true;
    };

    ~Logger();
private:
    void enqueueWrite(const QString& message, Level level);
    void write(const QString& message, Level level);
    friend class LogWriterRunnable;
protected:
    LoggerImpl* d;
};
} // end namespace
BIND_TO_SELF_SINGLE(QsLogging::Logger);

//(std::ostringstream() << std::this_thread::get_id()).str();

inline QString idToStr(std::thread::id id)
{
    std::ostringstream ss;
    ss << id;
    std::string idstr = ss.str();
    return QString::fromStdString(idstr);
}

//! Logging macros: define QS_LOG_LINE_NUMBERS to get the file and line number
//! in the log output.
#ifndef QS_LOG_LINE_NUMBERS
#define QLOG_TRACE() QsLogging::Logger::Helper(QsLogging::TraceLevel, An<QsLogging::Logger>().getData()).stream() << idToStr(std::this_thread::get_id())  << " "
#define QLOG_DEBUG() QsLogging::Logger::Helper(QsLogging::DebugLevel, An<QsLogging::Logger>().getData()).stream() << idToStr(std::this_thread::get_id())  << " "
#define QLOG_INFO() QsLogging::Logger::Helper(QsLogging::InfoLevel, An<QsLogging::Logger>().getData()).stream() << idToStr(std::this_thread::get_id())  << " "
#define QLOG_WARN()  QsLogging::Logger::Helper(QsLogging::WarnLevel,  An<QsLogging::Logger>().getData()).stream() << idToStr(std::this_thread::get_id())  << " "
#define QLOG_ERROR() QsLogging::Logger::Helper(QsLogging::ErrorLevel, An<QsLogging::Logger>().getData()).stream() << idToStr(std::this_thread::get_id())  << " "
#define QLOG_FATAL() QsLogging::Logger::Helper(QsLogging::FatalLevel, An<QsLogging::Logger>().getData()).stream() << idToStr(std::this_thread::get_id())  << " "
#else
#define QLOG_TRACE() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::TraceLevel) {} \
    else  QsLogging::Logger::Helper(QsLogging::TraceLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_DEBUG() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::DebugLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::DebugLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_INFO()  \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::InfoLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::InfoLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_WARN()  \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::WarnLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::WarnLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_ERROR() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::ErrorLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::ErrorLevel).stream() << __FILE__ << '@' << __LINE__
#define QLOG_FATAL() \
    if (QsLogging::Logger::instance().loggingLevel() > QsLogging::FatalLevel) {} \
    else QsLogging::Logger::Helper(QsLogging::FatalLevel).stream() << __FILE__ << '@' << __LINE__
#endif

#ifdef QS_LOG_DISABLE
#include "QsLogDisableForThisFile.h"
#endif

#endif // QSLOG_H
