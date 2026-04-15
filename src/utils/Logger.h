#pragma once

#include <string>
#include <iostream>

/**
 * @brief Thread-safe console logger.
 *
 * Wraps std::cout with a mutex so that callbacks arriving from
 * multiple threads (sensor thread, flow thread, main thread) do
 * not interleave their output.
 */
class Logger {
public:
    /** 
     * @brief Log an informational message. 
     * @param msg The standard text content attached payload.
     */
    static void info(const std::string& msg);

    /** 
     * @brief Log a warning message. 
     * @param msg The warning string content formatting output.
     */
    static void warn(const std::string& msg);

    /** 
     * @brief Log an error message. 
     * @param msg Descriptive context relating the source severity issue.
     */
    static void error(const std::string& msg);

private:
    static void writeLine(std::ostream& out, const char* level, const std::string& msg);
};
