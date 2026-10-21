#include "recon_all.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

namespace {

constexpr std::array<const char *, 11> kLogLevelNames = {
    "GAME   ", "ERROR  ", "WARNING", "INFO   ", " SERVER", "CLIENT ",
    "DEBUG  ", "DEBUG1 ", "DEBUG2 ", "DEBUG3 ", "DEBUG4 "};

constexpr int kLogFilesToRotate = 10;
constexpr std::uintmax_t kMaximumLogBodySize = 1000000;
constexpr const char *kLogCutMarker =
    "\n\n. . LOG FILE CUT . .\n\n. . LOG FILE CUT . .\n\n. . LOG FILE CUT . .\n\n";

LogLevel &logLevelStorage() {
    static LogLevel level = LOG_RUN;
    return level;
}

void writeToLogFile(_iobuf *file, const std::string &text) {
    if (file && !text.empty()) {
        std::fwrite(text.data(), 1, text.size(), reinterpret_cast<std::FILE *>(file));
    }
}

std::string logPath(const char *directory, int index) {
    if (index == 0) {
        return std::string(directory) + "log.log";
    }
    return std::string(directory) + "log" + std::to_string(index) + ".log";
}

} // namespace

Log::Log() : messageLevel(LOG_RUN) {}

Log::~Log() {
    // Buffered errors go to stdout and the log file first (original: fprintf(stdout/logFile, "%s")).
    const std::string pendingErrors = errorBuffer.str();
    if (!pendingErrors.empty()) {
        std::fputs(pendingErrors.c_str(), stdout);
        writeToLogFile(logFile, pendingErrors);
    }
    errorBuffer.str(std::string());
    errorBuffer.clear();

    os << std::endl;
    writeToLogFile(logFile, os.str());
    if (logFile) {
        std::fflush(reinterpret_cast<std::FILE *>(logFile));
    }
}

std::basic_ostringstream<char> &Log::get(LogLevel level) {
    if (level == LOG_RUN) {
        os << "                      ";
        return os;
    }

    const auto levelIndex = static_cast<unsigned int>(level);
    const char *levelName = levelIndex < kLogLevelNames.size()
                                ? kLogLevelNames[levelIndex]
                                : "UNKNOWN";

    static const auto start = std::chrono::steady_clock::now();
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() % 1000;

    static std::mutex localTimeMutex;
    {
        std::lock_guard<std::mutex> lock(localTimeMutex);
        const auto localTime = std::localtime(&time);
        if (localTime) {
            os << std::put_time(localTime, "%H:%M:%S");
        } else {
            os << "Error in NowTime()";
        }
    }
    os << '.' << std::setw(3) << std::setfill('0') << milliseconds << std::setfill(' ')
       << ' ' << levelName << ": ";

    if (levelIndex > LOG_DEBUG) {
        os << std::string(levelIndex - LOG_DEBUG, '\t');
    }
    return os;
}

std::basic_ostringstream<char> &Log::getConditional(
    bool condition, bool isClient, LogLevel, LogLevel) {
    if (condition) {
        return get(LOG_SERVER);
    }
    if (!isClient) {
        return get(LOG_DEBUG);
    }
    return get(LOG_CLIENT);
}

void Log::ensureLogDirectoryExists() {
    std::error_code error;
    std::filesystem::create_directories(LOG_DIRECTORY, error);
    if (error || !std::filesystem::is_directory(LOG_DIRECTORY, error)) {
        std::cerr << "Log::ensureLogDirectoryExists() - Unable to create directory: "
                  << LOG_DIRECTORY << '\n';
    }
}

void Log::init(LogLevel) {
    reportingLevel() = LOG_DEBUG4;
    if (logFile) {
        std::cerr << "Log::init() - Logger already initialized. Aborting...\n";
        return;
    }

    ensureLogDirectoryExists();
    std::error_code error;
    for (int index = kLogFilesToRotate - 1; index >= 0; --index) {
        const auto source = logPath(LOG_DIRECTORY, index);
        const auto destination = logPath(LOG_DIRECTORY, index + 1);
        error.clear();
        if (!std::filesystem::exists(source, error) || error) {
            continue;
        }

        if (index == kLogFilesToRotate - 1) {
            error.clear();
            std::filesystem::remove(destination, error);
            if (error) {
                std::cerr << "Log::init() - Unable to remove file " << destination << ".\n";
                continue;
            }
        }

        error.clear();
        std::filesystem::rename(source, destination, error);
        if (error) {
            std::cerr << "Log::init() - Unable to rename file " << source << " to "
                      << destination << ".\n";
        }
    }

    logFileName = logPath(LOG_DIRECTORY, 0);
    logFile = reinterpret_cast<_iobuf *>(
        std::fopen(logFileName.c_str(), "w"));
    if (!logFile) {
        std::cerr << "Log::init() - Unable to initialize logger, cannot create log file.\n";
    }
}

LogLevel &Log::reportingLevel() {
    return logLevelStorage();
}

std::string Log::toString(LogLevel level) {
    const auto index = static_cast<unsigned int>(level);
    if (index >= kLogLevelNames.size()) {
        return "UNKNOWN";
    }
    return kLogLevelNames[index];
}

LogLevel Log::fromString(const std::string &name) {
    const auto trim = [](std::string value) {
        const auto first = value.find_first_not_of(' ');
        if (first == std::string::npos) {
            return std::string();
        }
        const auto last = value.find_last_not_of(' ');
        return value.substr(first, last - first + 1);
    };
    const auto normalizedName = trim(name);
    for (unsigned int index = 0; index < kLogLevelNames.size(); ++index) {
        if (normalizedName == trim(kLogLevelNames[index])) {
            return static_cast<LogLevel>(index);
        }
    }
    return LOG_RUN;
}

std::string Log::closeLogAndGetBody() {
    if (logFile) {
        std::fclose(reinterpret_cast<std::FILE *>(logFile));
        logFile = nullptr;
    }

    std::ifstream input(logFileName, std::ios::binary);
    if (!input) {
        return {};
    }

    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    const auto fileSize = static_cast<std::streamoff>(end);
    if (fileSize <= 0) {
        return {};
    }
    input.seekg(0, std::ios::beg);

    if (fileSize < kMaximumLogBodySize + 1) {
        return std::string(std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>());
    }

    std::string body;
    std::string line;
    for (int count = 0; count < 100 && std::getline(input, line); ++count) {
        body += line;
        body += '\n';
    }
    const auto firstHundredLinesEnd = input.tellg();
    const auto tailStart = std::max<std::streamoff>(
        fileSize - static_cast<std::streamoff>(kMaximumLogBodySize),
        firstHundredLinesEnd < 0 ? 0 : static_cast<std::streamoff>(firstHundredLinesEnd));
    body += kLogCutMarker;
    input.clear();
    input.seekg(tailStart, std::ios::beg);
    body.append(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return body;
}
