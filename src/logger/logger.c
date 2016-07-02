/*
 * logger.c - log module
 *
 * author       : Jeroen van der Heijden
 * email        : jeroen@transceptor.technology
 * copyright    : 2016, Transceptor Technology
 *
 * changes
 *  - initial version, 08-03-2016
 *
 */
#include <logger/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

logger_t Logger = {
        .level=10,
        .level_name=NULL,
        .ostream=NULL,
        .flags=0
};

#define LOGGER_CHR_MAP "DIWECU"

#define KNRM  "\x1B[0m"     // normal
#define KRED  "\x1B[31m"    // error
#define KGRN  "\x1B[32m"    // info
#define KYEL  "\x1B[33m"    // warning
#define KBLU  "\x1B[34m"    // -- not used --
#define KMAG  "\x1B[35m"    // critical
#define KCYN  "\x1B[36m"    // debug
#define KWHT  "\x1B[37m"    // -- not used --

const char * LOGGER_LEVEL_NAMES[LOGGER_NUM_LEVELS] =
    {"debug", "info", "warning", "error", "critical"};

#define LOGGER_LOG_STUFF(LEVEL)                                 \
{                                                               \
    time_t t = time(NULL);                                      \
    struct tm tm = *localtime(&t);                              \
    if (Logger.flags & LOGGER_FLAG_COLORED)                     \
    {                                                           \
        char * color =                                          \
                (LEVEL <= 10) ? KCYN :                          \
                (LEVEL <= 20) ? KGRN :                          \
                (LEVEL <= 30) ? KYEL :                          \
                (LEVEL <= 40) ? KRED : KMAG;                    \
        fprintf(Logger.ostream,                                 \
            "%s[%c %d-%0*d-%0*d %0*d:%0*d:%0*d]" KNRM " ",      \
            color,                                              \
            LOGGER_CHR_MAP[(LEVEL - 1) / 10],                   \
            tm.tm_year + 1900,                                  \
            2, tm.tm_mon + 1,                                   \
            2, tm.tm_mday,                                      \
            2, tm.tm_hour,                                      \
            2, tm.tm_min,                                       \
            2, tm.tm_sec);                                      \
    }                                                           \
    else                                                        \
    {                                                           \
        fprintf(Logger.ostream,                                 \
        "[%c %d-%0*d-%0*d %0*d:%0*d:%0*d] ",                    \
            LOGGER_CHR_MAP[(LEVEL - 1) / 10],                   \
            tm.tm_year + 1900,                                  \
            2, tm.tm_mon + 1,                                   \
            2, tm.tm_mday,                                      \
            2, tm.tm_hour,                                      \
            2, tm.tm_min,                                       \
            2, tm.tm_sec);                                      \
    }                                                           \
    /* print the actual log line */                             \
    va_list args;                                               \
    va_start(args, fmt);                                        \
    vfprintf(Logger.ostream, fmt, args);                        \
    va_end(args);                                               \
    /* write end of line and flush the stream */                \
    fputc('\n', Logger.ostream);                                \
    fflush(Logger.ostream);                                     \
}

/*
 * Initialize the Logger.
 */
void logger_init(struct _IO_FILE * ostream, int log_level)
{
    Logger.ostream = ostream;
    logger_set_level(log_level);
}

/*
 * Set the logger to a given level. (name will be set too)
 */
void logger_set_level(int log_level)
{
    Logger.level = log_level;
    Logger.level_name = logger_level_name(log_level);
}

/*
 * Returns a log level name for a given log level.
 */
const char * logger_level_name(int log_level)
{
    return LOGGER_LEVEL_NAMES[(log_level - 1) / 10];
}

void log__debug(char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_DEBUG)

void log__info(char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_INFO)

void log__warning(char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_WARNING)

void log__error(char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_ERROR)

void log__critical(char * fmt, ...)
    LOGGER_LOG_STUFF(LOGGER_CRITICAL)

