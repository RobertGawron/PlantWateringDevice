#ifndef LOGGER_H
#define LOGGER_H

#ifndef TARGET_HOST

/* No-op on target hardware */
#define logDebugLow(fmt, ...)
#define logDebugHigh(fmt, ...)
#define logInfo(fmt, ...)
#define logWarning(fmt, ...)
#define logError(fmt, ...)

#else /* TARGET_HOST */

#include <emscripten.h>
#include <stdarg.h>
#include <stdio.h>

/** Maximum length of a single formatted log message */
#define LOG_MESSAGE_MAX_LEN 256U

/** Log severity levels */
typedef enum
{
    LOG_LEVEL_DEBUG_LOW = 0,
    LOG_LEVEL_DEBUG_HIGH,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_COUNT /* Must remain last     */
} LogLevel;

/**
 * @brief Format and send a log message to the web interface.
 *
 * @param level  Severity level
 * @param format printf-style format string
 */
static inline void logMessage(LogLevel level, const char *format, ...)
{
    static const char *const LEVEL_STRINGS[LOG_LEVEL_COUNT] = {
        [LOG_LEVEL_DEBUG_LOW] = "debug-low",
        [LOG_LEVEL_DEBUG_HIGH] = "debug-high",
        [LOG_LEVEL_INFO] = "info",
        [LOG_LEVEL_WARNING] = "warning",
        [LOG_LEVEL_ERROR] = "error"};

    char buffer[LOG_MESSAGE_MAX_LEN];
    va_list args;

    if ((unsigned int)level >= (unsigned int)LOG_LEVEL_COUNT)
    {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    MAIN_THREAD_EM_ASM({
        if (typeof addLog === 'function') {
            addLog(UTF8ToString($0), UTF8ToString($1));
        } }, buffer, LEVEL_STRINGS[level]);
}

#define logDebugLow(fmt, ...) logMessage(LOG_LEVEL_DEBUG_LOW, (fmt), ##__VA_ARGS__)
#define logDebugHigh(fmt, ...) logMessage(LOG_LEVEL_DEBUG_HIGH, (fmt), ##__VA_ARGS__)
#define logInfo(fmt, ...) logMessage(LOG_LEVEL_INFO, (fmt), ##__VA_ARGS__)
#define logWarning(fmt, ...) logMessage(LOG_LEVEL_WARNING, (fmt), ##__VA_ARGS__)
#define logError(fmt, ...) logMessage(LOG_LEVEL_ERROR, (fmt), ##__VA_ARGS__)

#endif /* TARGET_HOST */

#endif /* LOGGER_H */