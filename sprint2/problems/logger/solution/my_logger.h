#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
public:
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }
        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        std::ostringstream oss;
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        oss << std::put_time(std::localtime(&t_c), "/var/log/sample_log_%Y_%m_%d.log");
        return oss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard g(mutex_);
        std::ofstream log_file{GetFileTimeStamp(), std::ios::app};
        //
        log_file << GetTimeStamp() << ": "sv ;
        // Выводим аргументы функции, если они не пустые
        if constexpr (sizeof...(args) != 0) {
            LogImpl(log_file, args...);
        }
        log_file << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard g(mutex_);
        manual_ts_ = ts;
    }
    
private:
    template <typename T0, typename... Ts>
    void LogImpl(std::ofstream& out, const T0& arg0, const Ts&... args) {
        using namespace std::literals;
        out << arg0;
        // Выводим через запятую остальные параметры, если они остались
        if constexpr (sizeof...(args) != 0) {
            LogImpl(out, args...);  // Рекурсивно выводим остальные параметры
        }
    }
    
private:
    std::mutex mutex_;
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
};
