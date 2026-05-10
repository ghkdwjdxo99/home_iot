#include "buffer_manager.h"
#include "cam_path.h"
#include "cam_config.h"
#include "log/log.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static uint64_t g_segment_index = 0;
static bool g_segment_started = false;

static std::chrono::steady_clock::time_point g_segment_start_time;
static std::chrono::steady_clock::time_point g_last_write_time;

// 현재 시간을 문자열로 만든다.
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

// buffer segment 파일 이름을 만든다.
// buffer 내부 segment는 프로그램 실행 중 계속 증가하며 12자리로 관리한다.
static fs::path make_segment_path(uint64_t index)
{
    std::ostringstream oss;
    oss << "seg_" << std::setw(12) << std::setfill('0') << index << ".txt";

    return cam_path::BUFFER_DIR / oss.str();
}

// pre-motion 시간 기준으로 유지해야 할 segment 개수를 계산한다.
static std::size_t get_keep_segment_count()
{
    if (CAM_SEGMENT_SECONDS <= 0) {
        return 1;
    }

    const int count =
        (CAM_PRE_MOTION_SECONDS + CAM_SEGMENT_SECONDS - 1) / CAM_SEGMENT_SECONDS;

    return count > 0 ? static_cast<std::size_t>(count) : 1;
}

// 완료된 segment 파일인지 확인한다.
static bool is_segment_file(const fs::path& path)
{
    const std::string filename = path.filename().string();

    return filename.rfind("seg_", 0) == 0 &&
           path.extension() == ".txt";
}

// buffer에 있는 완료 segment 목록을 반환한다.
std::vector<fs::path> get_buffer_segments()
{
    std::vector<fs::path> segments;

    if (!fs::exists(cam_path::BUFFER_DIR)) {
        return segments;
    }

    for (const auto& entry : fs::directory_iterator(cam_path::BUFFER_DIR)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (is_segment_file(entry.path())) {
            segments.push_back(entry.path());
        }
    }

    std::sort(segments.begin(), segments.end());

    return segments;
}

// 오래된 buffer segment를 삭제하고 최근 segment만 유지한다.
bool cleanup_buffer_segments()
{
    try {
        std::vector<fs::path> segments = get_buffer_segments();
        const std::size_t keep_count = get_keep_segment_count();

        if (segments.size() <= keep_count) {
            return true;
        }

        const std::size_t remove_count = segments.size() - keep_count;

        for (std::size_t i = 0; i < remove_count; ++i) {
            fs::remove(segments[i]);
        }

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to cleanup old buffer segments: ") + e.what());
        return false;
    }
}

// buffer 디렉터리에 남아 있는 이전 실행의 임시 파일들을 삭제한다.
// buffer는 최근 영상 임시 보관 공간이므로 프로그램 시작 시 초기화한다.
static bool cleanup_buffer_directory()
{
    try {
        if (!fs::exists(cam_path::BUFFER_DIR)) {
            fs::create_directories(cam_path::BUFFER_DIR);
            return true;
        }

        for (const auto& entry : fs::directory_iterator(cam_path::BUFFER_DIR)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const fs::path path = entry.path();
            const std::string filename = path.filename().string();

            if (filename.rfind("seg_", 0) == 0 && path.extension() == ".txt") {
                fs::remove(path);
                continue;
            }

            if (path == cam_path::CURRENT_SEGMENT_TMP_FILE) {
                fs::remove(path);
                continue;
            }
        }

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to cleanup buffer directory: ") + e.what());
        return false;
    }
}

// 새 current segment 작성을 시작한다.
static bool start_new_current_segment()
{
    try {
        if (fs::exists(cam_path::CURRENT_SEGMENT_TMP_FILE)) {
            fs::remove(cam_path::CURRENT_SEGMENT_TMP_FILE);
        }

        std::ofstream ofs(cam_path::CURRENT_SEGMENT_TMP_FILE, std::ios::app);

        if (!ofs.is_open()) {
            log_write("cam", LogLevel::ERROR,
                      "failed to open current segment tmp");
            return false;
        }

        ofs << "segment_start=" << get_current_datetime() << std::endl;

        g_segment_start_time = std::chrono::steady_clock::now();
        g_last_write_time = g_segment_start_time;
        g_segment_started = true;

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to start current segment: ") + e.what());
        return false;
    }
}

// current segment에 주기적으로 한 줄 기록한다.
static bool write_current_segment_tick()
{
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_write_time).count();

    if (elapsed_ms < CAM_BUFFER_WRITE_INTERVAL_MS) {
        return true;
    }

    std::ofstream ofs(cam_path::CURRENT_SEGMENT_TMP_FILE, std::ios::app);

    if (!ofs.is_open()) {
        log_write("cam", LogLevel::ERROR,
                  "failed to write current segment tmp");
        return false;
    }

    ofs << get_current_datetime() << std::endl;
    g_last_write_time = now;

    return true;
}

// current segment를 완료 segment 파일로 확정한다.
static bool finalize_current_segment(BufferUpdateResult* result)
{
    if (result != nullptr) {
        result->segment_completed = false;
        result->completed_segment.clear();
    }

    if (!g_segment_started) {
        return start_new_current_segment();
    }

    try {
        {
            std::ofstream ofs(cam_path::CURRENT_SEGMENT_TMP_FILE, std::ios::app);

            if (!ofs.is_open()) {
                log_write("cam", LogLevel::ERROR,
                          "failed to finalize current segment tmp");
                return false;
            }

            ofs << "segment_end=" << get_current_datetime() << std::endl;
        }

        const fs::path completed_segment = make_segment_path(g_segment_index++);

        if (fs::exists(completed_segment)) {
            fs::remove(completed_segment);
        }

        fs::rename(cam_path::CURRENT_SEGMENT_TMP_FILE, completed_segment);

        if (result != nullptr) {
            result->segment_completed = true;
            result->completed_segment = completed_segment;
        }

        g_segment_started = false;

        return start_new_current_segment();
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to finalize current segment: ") + e.what());
        return false;
    }
}

// buffer manager를 초기화한다.
bool init_buffer_manager()
{
    try {
        fs::create_directories(cam_path::BUFFER_DIR);

        if (!fs::is_directory(cam_path::BUFFER_DIR)) {
            log_write("cam", LogLevel::ERROR,
                      "buffer path is not a directory: " +
                      cam_path::BUFFER_DIR.string());
            return false;
        }

        if (!cleanup_buffer_directory()) {
            return false;
        }

        g_segment_index = 0;
        g_segment_started = false;

        return start_new_current_segment();
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("buffer manager init failed: ") + e.what());
        return false;
    }
}

// current segment를 갱신하고, 시간이 지나면 완료 segment로 확정한다.
bool update_buffer_manager(BufferUpdateResult* result)
{
    if (result != nullptr) {
        result->segment_completed = false;
        result->completed_segment.clear();
    }

    if (!g_segment_started) {
        if (!start_new_current_segment()) {
            return false;
        }
    }

    if (!write_current_segment_tick()) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(now - g_segment_start_time).count();

    if (elapsed_seconds >= CAM_SEGMENT_SECONDS) {
        if (!finalize_current_segment(result)) {
            return false;
        }
    }

    return true;
}

// 현재 작성 중인 segment를 즉시 확정한다.
bool force_finalize_current_segment(BufferUpdateResult* result)
{
    if (!g_segment_started) {
        return true;
    }

    return finalize_current_segment(result);
}
