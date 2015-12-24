#ifndef LOGGERHELPER_H
#define LOGGERHELPER_H
#include "include/l_tree_controller_global.h"
#include <QString>

struct L_TREE_CONTROLLER_EXPORT LoggerHelper
{
    LoggerHelper();
    static void SaveLoggerStatus();
    static void RestoreLoggerStatus();
    static void SplitFile();
    static void InitLogger(int logLevel);
    static void SetParams(QString,QString,QString, int);
    static bool traceLevel;
    static bool debugLevel;
    static bool infoLevel;
    static bool warnLevel;
    static bool errorLevel;
    static bool criticalLevel;
    static QString logFileName;
    static QString logFileExtension;
    static QString logFilePath;
    static int maxSize;
};
#endif // LOGGERHELPER_H
