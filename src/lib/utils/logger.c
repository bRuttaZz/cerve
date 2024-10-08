#include "../../../include/utils.h"
#include "../../../include/server-configs.h"
#include <stdio.h>
#include <string.h>

void _debug(const char *msg);
void _info(const char *msg);
void _warning(const char *msg);
void _error(const char *msg);

// initializing logger with default values
struct Logger g_logger = {.level=99, .debug=_debug, .info=_info, .warning=_warning, .error=_error};


void _logger(const char* msg, const char* prefix, enum LogLevel level) {
    if (level > g_logger.level)
        return;
    size_t len = strlen(msg);
    char log_fmt[] = " %s\t%s";
    char resp[len + strlen(prefix) + strlen(log_fmt) + 2];

    sprintf(resp, log_fmt, prefix, msg);
    if (len <= 0 || msg[len - 1] != '\n')        // just for compatability with convention
        strcat(resp, "\n");

    if (level)
        fprintf(stdout, "%s", resp);
    else if (DBUG_MODE)
        perror(resp);
    else // log error in std err
        fprintf(stderr, "%s", resp);
}

// different loggers
void _debug(const char *msg) {
    _logger(msg, "\033[34mDEBUG\033[0m", LOG_LEVEL_DEBUG);
}
void _info(const char *msg) {
    _logger(msg, "\033[32mINFO\033[0m", LOG_LEVEL_INFO);
}
void _warning(const char *msg) {
    _logger(msg, "\033[33mWARNING\033[0m", LOG_LEVEL_WARNING);
}
void _error(const char *msg) {
    _logger(msg, "\033[31mERROR\033[0m", LOG_LEVEL_ERROR);
}
