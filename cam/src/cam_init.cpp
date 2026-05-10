#include "cam_init.h"
#include "log/log.h"
#include "cam_path.h"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// 입력받은 경로가 디렉터리인지 확인한다.
// 디렉터리가 없으면 새로 생성한다.
static bool ensure_directory(const fs::path& path)
{
    try {
        // 경로가 이미 존재하는 경우
        if (fs::exists(path)) {
            if (fs::is_directory(path)) {
                log_write("cam", LogLevel::INFO,
                          "directory exists: " + path.string());
                return true;
            }

            // 경로는 존재하지만 디렉터리가 아닌 경우
            log_write("cam", LogLevel::ERROR,
                      "path exists but is not a directory: " + path.string());
            return false;
        }

        // 필요한 디렉터리를 생성한다.
        // 중간 경로가 없으면 함께 생성된다.
        if (fs::create_directories(path)) {
            log_write("cam", LogLevel::INFO,
                      "directory created: " + path.string());
            return true;
        }

        // create_directories()가 false를 반환한 경우
        log_write("cam", LogLevel::ERROR,
                  "failed to create directory: " + path.string());
        return false;
    }
    catch (const fs::filesystem_error& e) {
        // 파일 시스템 처리 중 예외가 발생한 경우
        log_write("cam", LogLevel::ERROR,
                  std::string("filesystem error: ") + e.what());
        return false;
    }
}

// homecam 실행에 필요한 디렉터리를 준비한다.
bool init_homecam()
{
    // homecam 서비스 실행에 필요한 디렉터리 목록
    const std::vector<fs::path> required_dirs = {
        cam_path::BUFFER_DIR,
        cam_path::CONFIG_DIR,
        cam_path::RECORDINGS_DIR,
        cam_path::RECORDINGS_PENDING_DIR,
        cam_path::RECORDINGS_SYNCED_DIR,
        cam_path::RECORDINGS_FAILED_DIR
    };

    bool success = true;

    // 필요한 디렉터리가 모두 존재하는지 확인하고, 없으면 생성한다.
    for (const auto& dir : required_dirs) {
        if (!ensure_directory(dir)) {
            success = false;
        }
    }

    return success;
}