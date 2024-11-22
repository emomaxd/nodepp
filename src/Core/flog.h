#ifndef FLOG_H
#define FLOG_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <utility>
#include <iomanip>
#include <fstream>

#include "ff.h"

namespace flog {

// Log levels
enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };

// Colors (ANSI escape codes)
enum class Color {
    RESET = 0,
    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
};

// Thread Pool for Async Logging
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount) : stop(false) {
        for (size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop)
                throw std::runtime_error("ThreadPool is stopped.");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return task->get_future();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// Logger
class Logger {
public:
    explicit Logger(const std::string &name, std::ostream &outStream = std::cout)
        : name(name), outStream(outStream), isAsync(false), threadPool(nullptr), backtraceThreshold(32), messageCount(0), 
          periodicFlushInterval(std::chrono::seconds(5)), fileFlushThreshold(1024 * 1024), currentFileSize(0), isFileLogging(false) {}

    void enableAsync(ThreadPool &pool) {
        threadPool = &pool;
        isAsync = true;
    }

    // Enable file logging with rotation size
    void enableFileLogging(const std::string &filename, size_t rotationSize = 1024 * 1024 * 1024) {
        isFileLogging = true;
        logFile.open(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            isFileLogging = false;
        }
        fileFlushThreshold = rotationSize;  // Set the rotation size
    }

    // Method to set file rotation size
    void setFileRotationSize(size_t size) {
        fileFlushThreshold = size;
    }

    void setPeriodicFlush(std::chrono::seconds interval) {
        periodicFlushInterval = interval;
        startPeriodicFlush();
    }

    void setBacktraceThreshold(size_t threshold) {
        backtraceThreshold = threshold;
    }

    // Log function using ff::format with parameter forwarding
    template <typename... Args>
    void log(Level level, const std::string &message, Args&&... args) {
        std::string formattedMessage = formatMessage(level, message);

        // Use ff::format if parameters exist
        if constexpr (sizeof...(args) > 0) {
            formattedMessage = ff::format(formattedMessage.c_str(), std::forward<Args>(args)...);
        }

        if (isAsync && threadPool) {
            threadPool->enqueue([this, level, formattedMessage] {
                logToStream(level, formattedMessage);
            });
        } else {
            logToStream(level, formattedMessage);
        }
    }

    void flush() {
        if (isAsync && threadPool) {
            threadPool->enqueue([this] {
                logFile.flush();
                outStream.flush();
            });
        } else {
            logFile.flush();
            outStream.flush();
        }
    }

    // Trace level log
    template <typename... Args>
    void trace(const std::string &message, Args&&... args) {
        log(Level::TRACE, message, std::forward<Args>(args)...);
    }

    // Debug level log
    template <typename... Args>
    void debug(const std::string &message, Args&&... args) {
        log(Level::DEBUG, message, std::forward<Args>(args)...);
    }

    // Info level log
    template <typename... Args>
    void info(const std::string &message, Args&&... args) {
        log(Level::INFO, message, std::forward<Args>(args)...);
    }

    // Warn level log
    template <typename... Args>
    void warn(const std::string &message, Args&&... args) {
        log(Level::WARN, message, std::forward<Args>(args)...);
    }

    // Error level log
    template <typename... Args>
    void error(const std::string &message, Args&&... args) {
        log(Level::ERROR, message, std::forward<Args>(args)...);
    }

    // Critical level log
    template <typename... Args>
    void critical(const std::string &message, Args&&... args) {
        log(Level::CRITICAL, message, std::forward<Args>(args)...);
    }

private:
    void logToStream(Level level, const std::string &formattedMessage) {
        std::lock_guard<std::mutex> lock(mutex);
        if (isFileLogging) {
            if (currentFileSize >= fileFlushThreshold) {
                rotateLogFile();
            }
            logFile << getColorCode(level) << formattedMessage << getColorCode(Color::RESET) << std::endl;
            currentFileSize += formattedMessage.size();
        }
        outStream << getColorCode(level) << formattedMessage << getColorCode(Color::RESET) << std::endl;
        
        if (++messageCount >= backtraceThreshold) {
            flush();
            messageCount = 0;
        }
    }

    void startPeriodicFlush() {
        if (isAsync && threadPool) {
            threadPool->enqueue([this] {
                while (true) {
                    std::this_thread::sleep_for(periodicFlushInterval);
                    flush();
                }
            });
        }
    }

    void rotateLogFile() {
        // Rotate logic: rename the current log file and start a new one
        logFile.close();
        logFile.open("log_" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) + ".txt", std::ios::out | std::ios::app);
        currentFileSize = 0;
    }

    std::string formatMessage(Level level, const std::string &message) {
        std::ostringstream oss;
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        oss << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "]";
        oss << "[" << levelToString(level) << "] " << message;
        return oss.str();
    }

    std::string levelToString(Level level) {
        switch (level) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
        case Level::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
        }
    }

    std::string getColorCode(Level level) {
        switch (level) {
        case Level::TRACE: return "\033[" + std::to_string(static_cast<int>(Color::CYAN)) + "m";
        case Level::DEBUG: return "\033[" + std::to_string(static_cast<int>(Color::BLUE)) + "m";
        case Level::INFO: return "\033[" + std::to_string(static_cast<int>(Color::GREEN)) + "m";
        case Level::WARN: return "\033[" + std::to_string(static_cast<int>(Color::YELLOW)) + "m";
        case Level::ERROR: return "\033[" + std::to_string(static_cast<int>(Color::RED)) + "m";
        case Level::CRITICAL: return "\033[" + std::to_string(static_cast<int>(Color::MAGENTA)) + "m";
        default: return "\033[" + std::to_string(static_cast<int>(Color::RESET)) + "m";
        }
    }

    std::string getColorCode(Color color) {
        return "\033[" + std::to_string(static_cast<int>(color)) + "m";
    }

    std::string name;
    std::ostream &outStream;
    std::mutex mutex;
    bool isAsync;
    ThreadPool *threadPool;
    size_t backtraceThreshold;
    size_t messageCount;

    bool isFileLogging;
    std::ofstream logFile;
    size_t currentFileSize;
    size_t fileFlushThreshold;
    std::chrono::seconds periodicFlushInterval;
};

// Global Static Default Logger
inline Logger defaultLogger("defaultLogger", std::cout);

// Logger Manager
class LoggerManager {
public:
    static std::shared_ptr<Logger> createLogger(const std::string &name, std::ostream &outStream = std::cout) {
        auto logger = std::make_shared<Logger>(name, outStream);
        loggers[name] = logger;
        return logger;
    }

    static std::shared_ptr<Logger> getLogger(const std::string &name) {
        auto it = loggers.find(name);
        if (it != loggers.end())
            return it->second;
        return nullptr;
    }

    static void shutdown() {
        for (auto &[name, logger] : loggers) {
            logger->flush();
        }
        loggers.clear();
    }

private:
    static inline std::unordered_map<std::string, std::shared_ptr<Logger>> loggers;
};

// Logging functions using the default logger
template <typename... Args>
inline void trace(const std::string& message, Args&&... args) {
    defaultLogger.log(Level::TRACE, message, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(const std::string& message, Args&&... args) {
    defaultLogger.log(Level::DEBUG, message, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(const std::string& message, Args&&... args) {
    defaultLogger.log(Level::INFO, message, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warn(const std::string& message, Args&&... args) {
    defaultLogger.log(Level::WARN, message, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(const std::string& message, Args&&... args) {
    defaultLogger.log(Level::ERROR, message, std::forward<Args>(args)...);
}

template <typename... Args>
inline void critical(const std::string& message, Args&&... args) {
    defaultLogger.log(Level::CRITICAL, message, std::forward<Args>(args)...);
}

} // namespace flog

#endif // FLOG_H
