#include "utils/Logger.h"

void Logger::writeLine(std::ostream& out, const char* level, const std::string& msg) {
    out << "[" << level << "] " << msg << "\n";
}

void Logger::info(const std::string& msg) {
    writeLine(std::cout, "INFO ", msg);
}

void Logger::warn(const std::string& msg) {
    writeLine(std::cout, "WARN ", msg);
}

void Logger::error(const std::string& msg) {
    writeLine(std::cerr, "ERROR", msg);
}
