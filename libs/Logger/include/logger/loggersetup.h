#pragma once
#include "QsLog.h"

template<typename T>
void SetupLogger(QString loggerName, int logLevel, QString logFileFolder, bool rotate = false, int filesize = 512, int amountOfFilesToKeep = 50)
{
    An<T> logger;
    logger->clearDestinationList();
    logger->setLoggingLevel(static_cast<QsLogging::Level>(logLevel));
    if(logFileFolder.right(1) != "/")
        logFileFolder+="/";
    QString logFile = logFileFolder + loggerName + ".log";

    QsLogging::DestinationPtr fileDestination(
                QsLogging::DestinationFactory::MakeErrDumpDestination(logFile,rotate,filesize*1000000,amountOfFilesToKeep*1000000));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);
    QLOG_INFO() << QString("Starting %1 log").arg(loggerName);
}



