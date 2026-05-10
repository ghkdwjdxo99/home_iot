#include "log/log.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// 현재 날짜를 YYYY-MM-DD 형식으로 반환한다.
// 로그 파일 이름에 사용한다.
static std::string get_current_date()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
    localtime_r(&now_time, &local_time);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d");

    return oss.str();
}

// 현재 시간을 YYYY-MM-DD HH:MM:SS 형식으로 반환한다.
// 로그 한 줄의 timestamp에 사용한다.
static std::string get_current_datetime()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
    localtime_r(&now_time, &local_time);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

// LogLevel 값을 문자열로 변환한다.
static std::string log_level_to_string(LogLevel level)
{
    switch (level) {
    case LogLevel::INFO:
        return "INFO";

    case LogLevel::WARN:
        return "WARN";

    case LogLevel::ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}

// logs/ 및 logs/{module_name}/ 디렉터리를 준비한다.
static bool ensure_log_directory(const std::string& module_name)
{
    try {
        const fs::path log_dir = fs::path("logs") / module_name;

        if (fs::exists(log_dir)) {
            return fs::is_directory(log_dir);
        }

        return fs::create_directories(log_dir);
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "[LOG ERROR] failed to create log directory: "
                  << e.what() << std::endl;
        return false;
    }
}

// 지정한 모듈의 날짜별 로그 파일에 로그를 기록한다.
void log_write(const std::string& module_name,
               LogLevel level,
               const std::string& message)
{
    const std::string level_str = log_level_to_string(level);

    const std::string log_line =
        "[" + get_current_datetime() + "] " +
        "[" + module_name + "] " +
        "[" + level_str + "] " +
        message;

    // 터미널에도 로그를 출력한다.
    // ERROR는 표준 에러로 출력하고, 그 외는 표준 출력으로 출력한다.
    if (level == LogLevel::ERROR) {
        std::cerr << log_line << std::endl;
    } else {
        std::cout << log_line << std::endl;
    }

    // 로그 디렉터리가 없으면 생성한다.
    if (!ensure_log_directory(module_name)) {
        std::cerr << "[LOG ERROR] log directory is not ready: logs/"
                  << module_name << std::endl;
        return;
    }

    const fs::path log_file =
        fs::path("logs") / module_name / (get_current_date() + ".log");

    // append 모드로 연다.
    // 파일이 있으면 뒤에 추가하고, 없으면 새로 생성한다.
    std::ofstream ofs(log_file, std::ios::app);

    if (!ofs.is_open()) {
        std::cerr << "[LOG ERROR] failed to open log file: "
                  << log_file.string() << std::endl;
        return;
    }

    ofs << log_line << std::endl;
}