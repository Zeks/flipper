
#ifndef L_LOGGERSHARED_EXPORT_H
#define L_LOGGERSHARED_EXPORT_H

#ifdef LOGGER_STATIC_DEFINE
#  define L_LOGGERSHARED_EXPORT
#  define LOGGER_NO_EXPORT
#else
#  ifndef L_LOGGERSHARED_EXPORT
#    ifdef Logger_EXPORTS
        /* We are building this library */
#      define L_LOGGERSHARED_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define L_LOGGERSHARED_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef LOGGER_NO_EXPORT
#    define LOGGER_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef LOGGER_DEPRECATED
#  define LOGGER_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef LOGGER_DEPRECATED_EXPORT
#  define LOGGER_DEPRECATED_EXPORT L_LOGGERSHARED_EXPORT LOGGER_DEPRECATED
#endif

#ifndef LOGGER_DEPRECATED_NO_EXPORT
#  define LOGGER_DEPRECATED_NO_EXPORT LOGGER_NO_EXPORT LOGGER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LOGGER_NO_DEPRECATED
#    define LOGGER_NO_DEPRECATED
#  endif
#endif

#endif /* L_LOGGERSHARED_EXPORT_H */
