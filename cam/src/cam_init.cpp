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
        if (fs::exists(path)) {
            if (fs::is_directory(path)) {
                return true;
            }

            log_write("cam", LogLevel::ERROR,
                      "path exists but is not a directory: " + path.string());
            return false;
        }

        if (fs::create_directories(path)) {
            log_write("cam", LogLevel::INFO,
                      "directory created: " + path.string());
            return true;
        }

        log_write("cam", LogLevel::ERROR,
                  "failed to create directory: " + path.string());
        return false;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("filesystem error: ") + e.what());
        return false;
    }
}

// homecam 실행에 필요한 디렉터리를 준비한다.
bool init_homecam()
{
    const std::vector<fs::path> required_dirs = {
        cam_path::BUFFER_DIR,
        cam_path::CONFIG_DIR,
        cam_path::RECORDINGS_DIR,
        cam_path::RECORDINGS_PENDING_DIR,
        cam_path::RECORDINGS_SYNCED_DIR,
        cam_path::RECORDINGS_FAILED_DIR
    };

    bool success = true;

    for (const auto& dir : required_dirs) {
        if (!ensure_directory(dir)) {
            success = false;
        }
    }

    return success;
}