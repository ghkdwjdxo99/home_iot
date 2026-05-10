#include "buffer_manager.h"
#include "log/log.h"
#include "cam_path.h"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// 현재 시간을 문자열로 만든다.
// 더미 buffer 파일 내용에 기록하기 위한 용도다.
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

// 이전 실행 중 남아 있을 수 있는 임시 buffer 파일을 삭제한다.
// current.tmp는 작성 중 전원 차단이 발생했을 가능성이 있으므로 정상 파일로 취급하지 않는다.
static bool cleanup_tmp_buffer_file()
{
    try {
        if (fs::exists(cam_path::CURRENT_BUFFER_TMP_FILE)) {
            fs::remove(cam_path::CURRENT_BUFFER_TMP_FILE);

            log_write("cam", LogLevel::WARN,
                      "stale tmp buffer file removed: " +
                      cam_path::CURRENT_BUFFER_TMP_FILE.string());
        }

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to cleanup tmp buffer file: ") + e.what());
        return false;
    }
}

// 더미 current buffer 파일을 안전하게 생성한다.
// 먼저 current.tmp에 작성한 뒤, 작성 완료 후 current.txt로 이름을 변경한다.
static bool create_current_buffer_file()
{
    try {
        // 이전 tmp 파일이 남아 있으면 삭제한다.
        if (fs::exists(cam_path::CURRENT_BUFFER_TMP_FILE)) {
            fs::remove(cam_path::CURRENT_BUFFER_TMP_FILE);
        }

        {
            std::ofstream ofs(cam_path::CURRENT_BUFFER_TMP_FILE);

            if (!ofs.is_open()) {
                log_write("cam", LogLevel::ERROR,
                          "failed to create tmp buffer file: " +
                          cam_path::CURRENT_BUFFER_TMP_FILE.string());
                return false;
            }

            ofs << "dummy current buffer" << std::endl;
            ofs << "created_at=" << get_current_datetime() << std::endl;
        }

        // 기존 current.txt가 남아 있으면 제거한다.
        // rotate 과정에서는 보통 이미 prev로 이동되어 있어야 한다.
        if (fs::exists(cam_path::CURRENT_BUFFER_FILE)) {
            fs::remove(cam_path::CURRENT_BUFFER_FILE);
        }

        fs::rename(cam_path::CURRENT_BUFFER_TMP_FILE,
                   cam_path::CURRENT_BUFFER_FILE);

        log_write("cam", LogLevel::INFO,
                  "current buffer file created: " +
                  cam_path::CURRENT_BUFFER_FILE.string());

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to create current buffer file: ") + e.what());
        return false;
    }
}

// buffer manager를 초기화한다.
bool init_buffer_manager()
{
    try {
        if (!fs::exists(cam_path::BUFFER_DIR)) {
            fs::create_directories(cam_path::BUFFER_DIR);

            log_write("cam", LogLevel::INFO,
                      "buffer directory created: " +
                      cam_path::BUFFER_DIR.string());
        }

        if (!fs::is_directory(cam_path::BUFFER_DIR)) {
            log_write("cam", LogLevel::ERROR,
                      "buffer path is not a directory: " +
                      cam_path::BUFFER_DIR.string());
            return false;
        }

        // 이전 실행 중 남은 tmp 파일을 정리한다.
        if (!cleanup_tmp_buffer_file()) {
            return false;
        }

        // current.txt가 없으면 새로 만든다.
        if (!fs::exists(cam_path::CURRENT_BUFFER_FILE)) {
            return create_current_buffer_file();
        }

        log_write("cam", LogLevel::INFO,
                  "current buffer file exists: " +
                  cam_path::CURRENT_BUFFER_FILE.string());

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("buffer manager init filesystem error: ") + e.what());
        return false;
    }
}

// current.txt를 prev.txt로 교체하고 새로운 current.txt를 만든다.
bool rotate_buffer_file()
{
    try {
        log_write("cam", LogLevel::INFO, "buffer rotate start");

        // 혹시 이전 tmp 파일이 남아 있으면 삭제한다.
        if (!cleanup_tmp_buffer_file()) {
            return false;
        }

        // 기존 prev.txt는 삭제한다.
        if (fs::exists(cam_path::PREV_BUFFER_FILE)) {
            fs::remove(cam_path::PREV_BUFFER_FILE);

            log_write("cam", LogLevel::INFO,
                      "old prev buffer file removed: " +
                      cam_path::PREV_BUFFER_FILE.string());
        }

        // current.txt가 있으면 prev.txt로 이동한다.
        if (fs::exists(cam_path::CURRENT_BUFFER_FILE)) {
            fs::rename(cam_path::CURRENT_BUFFER_FILE,
                       cam_path::PREV_BUFFER_FILE);

            log_write("cam", LogLevel::INFO,
                      "current buffer moved to prev buffer");
        } else {
            log_write("cam", LogLevel::WARN,
                      "current buffer file does not exist before rotate");
        }

        // 새로운 current.txt를 tmp 기반으로 안전하게 만든다.
        if (!create_current_buffer_file()) {
            return false;
        }

        log_write("cam", LogLevel::INFO, "buffer rotate done");

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("buffer rotate filesystem error: ") + e.what());
        return false;
    }
}