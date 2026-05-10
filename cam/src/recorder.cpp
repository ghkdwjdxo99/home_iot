#include "recorder.h"
#include "cam_path.h"
#include "log/log.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static bool g_event_active = false;
static fs::path g_event_tmp_dir;
static fs::path g_event_final_dir;
static unsigned int g_event_segment_index = 0;

// 이벤트 디렉터리 이름에 사용할 시간 문자열을 만든다.
static std::string get_event_time_string()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
    localtime_r(&now_time, &local_time);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y%m%d_%H%M%S");

    return oss.str();
}

// event_info.txt에 기록할 시간 문자열을 만든다.
static std::string get_event_datetime_string()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
    localtime_r(&now_time, &local_time);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

// 고유한 이벤트 디렉터리 경로를 만든다.
static void make_unique_event_dirs()
{
    const std::string base_name = "event_" + get_event_time_string();

    g_event_final_dir = cam_path::RECORDINGS_PENDING_DIR / base_name;
    g_event_tmp_dir = cam_path::RECORDINGS_PENDING_DIR / (base_name + ".tmp");

    int index = 1;

    while (fs::exists(g_event_final_dir) || fs::exists(g_event_tmp_dir)) {
        const std::string name = base_name + "_" + std::to_string(index);

        g_event_final_dir = cam_path::RECORDINGS_PENDING_DIR / name;
        g_event_tmp_dir = cam_path::RECORDINGS_PENDING_DIR / (name + ".tmp");

        ++index;
    }
}

// 이벤트 내부 segment 파일 이름을 만든다.
// 이벤트마다 0부터 다시 시작한다.
static fs::path make_event_segment_path()
{
    std::ostringstream oss;
    oss << "seg_" << std::setw(6) << std::setfill('0')
        << g_event_segment_index++ << ".txt";

    return g_event_tmp_dir / oss.str();
}

// segment 파일을 현재 이벤트 임시 디렉터리로 복사한다.
static bool copy_segment_to_event(const fs::path& segment_path)
{
    try {
        if (!g_event_active) {
            return false;
        }

        if (!fs::exists(segment_path)) {
            log_write("cam", LogLevel::WARN,
                      "segment file missing: " + segment_path.string());
            return true;
        }

        const fs::path dst = make_event_segment_path();

        fs::copy_file(segment_path, dst, fs::copy_options::overwrite_existing);

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to copy segment to event: ") + e.what());
        return false;
    }
}

// event_info.txt를 생성한다.
static bool write_event_info()
{
    const fs::path info_file = g_event_tmp_dir / "event_info.txt";

    std::ofstream ofs(info_file);

    if (!ofs.is_open()) {
        log_write("cam", LogLevel::ERROR,
                  "failed to create event info file: " + info_file.string());
        return false;
    }

    ofs << "event_type=dummy_motion" << std::endl;
    ofs << "created_at=" << get_event_datetime_string() << std::endl;
    ofs << "segment_count=" << g_event_segment_index << std::endl;

    return true;
}

// event segment가 1개 이상 있는지 검사한다.
static bool has_event_segment()
{
    return g_event_segment_index > 0;
}

// motion event를 시작한다.
bool start_motion_event(const std::vector<fs::path>& pre_segments)
{
    try {
        if (g_event_active) {
            return true;
        }

        make_unique_event_dirs();

        fs::create_directories(g_event_tmp_dir);

        g_event_segment_index = 0;
        g_event_active = true;

        for (const auto& segment : pre_segments) {
            if (!copy_segment_to_event(segment)) {
                return false;
            }
        }

        log_write("cam", LogLevel::INFO,
                  "motion event started: " + g_event_tmp_dir.string());

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to start motion event: ") + e.what());
        g_event_active = false;
        return false;
    }
}

// motion 중 새로 완료된 segment를 추가한다.
bool add_motion_segment(const fs::path& segment_path)
{
    if (!g_event_active) {
        return true;
    }

    return copy_segment_to_event(segment_path);
}

// motion event를 확정한다.
bool finish_motion_event()
{
    try {
        if (!g_event_active) {
            return true;
        }

        if (!has_event_segment()) {
            log_write("cam", LogLevel::ERROR,
                      "motion event save failed: no segment in event");

            if (fs::exists(g_event_tmp_dir)) {
                fs::remove_all(g_event_tmp_dir);
            }

            g_event_active = false;
            g_event_tmp_dir.clear();
            g_event_final_dir.clear();
            g_event_segment_index = 0;

            return false;
        }

        if (!write_event_info()) {
            return false;
        }

        fs::rename(g_event_tmp_dir, g_event_final_dir);

        log_write("cam", LogLevel::INFO,
                  "motion event saved: " + g_event_final_dir.string());

        g_event_active = false;
        g_event_tmp_dir.clear();
        g_event_final_dir.clear();
        g_event_segment_index = 0;

        return true;
    }
    catch (const fs::filesystem_error& e) {
        log_write("cam", LogLevel::ERROR,
                  std::string("failed to finish motion event: ") + e.what());
        return false;
    }
}