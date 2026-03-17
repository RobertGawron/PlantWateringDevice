#ifndef LOGGER_H
#define LOGGER_H

#ifndef TARGET_HOST

/* No-op on target hardware */
#define WATERING_LOG_DEBUG_LOW(fmt, ...)
#define WATERING_LOG_DEBUG_HIGH(fmt, ...)
#define WATERING_LOG_INFO(fmt, ...)
#define WATERING_LOG_WARNING(fmt, ...)
#define WATERING_LOG_ERROR(fmt, ...)

#else /* TARGET_HOST */

#include <emscripten.h>
#include <stdarg.h>
#include <stdio.h>

/** Maximum length of a single formatted log message */
#define LOG_MESSAGE_MAX_LEN 256U

/** Log severity levels */
typedef enum
{
    WATERING_LOG_LEVEL_DEBUG_LOW = 0,
    WATERING_LOG_LEVEL_DEBUG_HIGH,
    WATERING_LOG_LEVEL_INFO,
    WATERING_LOG_LEVEL_WARNING,
    WATERING_LOG_LEVEL_ERROR,
    WATERING_LOG_LEVEL_COUNT /* Must remain last     */
} WateringLogLevel;

/**
 * @brief Format and send a log message to the web interface.
 *
 * @param level  Severity level
 * @param format printf-style format string
 */
static inline void watering_log_message(WateringLogLevel level, const char *format, ...)
{
    static const char *const LEVEL_STRINGS[WATERING_LOG_LEVEL_COUNT] = {
        [WATERING_LOG_LEVEL_DEBUG_LOW] = "debug-low",
        [WATERING_LOG_LEVEL_DEBUG_HIGH] = "debug-high",
        [WATERING_LOG_LEVEL_INFO] = "info",
        [WATERING_LOG_LEVEL_WARNING] = "warning",
        [WATERING_LOG_LEVEL_ERROR] = "error"};

    char buffer[LOG_MESSAGE_MAX_LEN];
    va_list args;

    if ((unsigned int)level >= (unsigned int)WATERING_LOG_LEVEL_COUNT)
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

#define WATERING_LOG_DEBUG_LOW(fmt, ...) watering_log_message(WATERING_LOG_LEVEL_DEBUG_LOW, (fmt), ##__VA_ARGS__)
#define WATERING_LOG_DEBUG_HIGH(fmt, ...) watering_log_message(WATERING_LOG_LEVEL_DEBUG_HIGH, (fmt), ##__VA_ARGS__)
#define WATERING_LOG_INFO(fmt, ...) watering_log_message(WATERING_LOG_LEVEL_INFO, (fmt), ##__VA_ARGS__)
#define WATERING_LOG_WARNING(fmt, ...) watering_log_message(WATERING_LOG_LEVEL_WARNING, (fmt), ##__VA_ARGS__)
#define WATERING_LOG_ERROR(fmt, ...) watering_log_message(WATERING_LOG_LEVEL_ERROR, (fmt), ##__VA_ARGS__)

#endif /* TARGET_HOST */

#endif /* LOGGER_H */