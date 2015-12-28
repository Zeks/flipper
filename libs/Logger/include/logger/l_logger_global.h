#ifndef L_LOGGER_GLOBAL_H
#define L_LOGGER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(L_LOGGER_LIBRARY)
#  define L_LOGGERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define L_LOGGERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // L_LOGGER_GLOBAL_H
