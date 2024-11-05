#include "../../../include/utils.h"
#include "../../../include/server-configs.h"
#include <bits/posix_opt.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

pthread_mutex_t _logging_lock = PTHREAD_MUTEX_INITIALIZER;

void _debug(const char *msg, ...);
void _info(const char *msg, ...);
void _warning(const char *msg, ...);
void _error(const char *msg, ...);

const char *_LOGGER_PREFIX[] = {     // index should be in the order of `enum LogLevel`
    "\033[31mERROR\033[0m",
    "\033[33mWARNING\033[0m",
    "\033[32mINFO\033[0m",
    "\033[34mDEBUG\033[0m",
};
const int _LOGGER_PREFIX_LEN = LOGGER_PREFIX_LEN;   // number of valid log level prefixes

// initializing logger with default values
struct Logger g_logger = {.level=DEFAULT_LOGLEVEL, .debug=_debug, .info=_info, .warning=_warning, .error=_error};


void _logger(enum LogLevel level, const char* msg, va_list format_args) {
    if (level > g_logger.level)
        return;

    pthread_mutex_lock(&_logging_lock);     // acquire lock to prevent intermediate fprintf exec from fellow threads

    FILE *outstream = level?stdout:stderr;
    int len_ = strlen(msg);

    if (level < _LOGGER_PREFIX_LEN)
        fprintf(outstream, " %s\t", _LOGGER_PREFIX[level]);
    else
        fprintf(outstream, " ");

    vfprintf(outstream, msg, format_args);
    if (msg != NULL && len_ > 0 && msg[len_-1] != '\n')        // just for compatability with convention
        fprintf(outstream, "\n");
    if (DBUG_MODE && !level)
        fprintf(stderr, ": %s\n", strerror(errno));

    pthread_mutex_unlock(&_logging_lock);
}

// different loggers
void _debug(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    _logger(LOG_LEVEL_DEBUG, msg, args);
}
void _info(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    _logger(LOG_LEVEL_INFO, msg, args);
}
void _warning(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    _logger(LOG_LEVEL_WARNING, msg, args);
}
void _error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    _logger(LOG_LEVEL_ERROR, msg, args);
}
