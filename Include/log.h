#pragma once

#include <defines.h>

#ifndef H_LOG
#define H_LOG

/**
 * @brief Different types of log messages and errors
 */
typedef enum error_type {
    __WARN__,   /**< Warning message */
    __ERROR__,  /**< Error message */
    __FATAL__,  /**< Fatal error, usually triggers exit */
    __LOG__     /**< General log message */
} error_t;

/**
 * @brief Opens the logging system and optionally redirects stderr
 * @param path Path to the log file
 * @param stderr_flag If true, also print log messages to stderr
 */
void open_logging(const char* path, bool stderr_flag);

/**
 * @brief Closes the logging system
 */
void close_logging();

/**
 * @brief Posts a log or error message with context
 * @param err Type of error or log (error_t)
 * @param file File name where the log is posted
 * @param line Line number where the log is posted
 * @param format printf-style format string
 * @param ... Additional arguments for the format string
 */
void __post_error(error_t err, const char* file, u32 line, const char* format, ...);

/**
 * @brief Sets a flag to indicate program should exit
 * @param flag True to indicate exit, false otherwise
 */
void set_exitFlag(bool flag);

/**
 * @brief Sets the exit code for the program
 * @param code Exit code
 */
void set_ExitCode(i32 code);

/**
 * @brief Forces the program to exit immediately, using the exit flag and exit code
 */
void error_exit();

/**
 * @brief Convenience macro to log a fatal error with file and line info
 */
#define logFatal(format, ...) __post_error(__FATAL__, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro to log an error with file and line info
 */
#define logError(format, ...) __post_error(__ERROR__, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro to log a warning with file and line info
 */
#define logWarn(format, ...) __post_error(__WARN__, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Convenience macro to log a general message with file and line info
 */
#define logInfo(format, ...) __post_error(__LOG__, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Convenience assert macro that logs an error and jumps to a cleanup label
 * @param statement Expression to check
 * @param cleanup Label to jump to if statement is true
 * @param format printf-style message if the assertion fails
 * @param ... Additional arguments for the format string
 */
#define assert(statement, cleanup, format, ...) if ((statement)) {logError(format, ##__VA_ARGS__); goto cleanup;}

#endif // H_LOG
