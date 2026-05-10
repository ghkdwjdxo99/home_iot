#include "motion_manager.h"
#include "buffer_manager.h"
#include "recorder.h"
#include "cam_config.h"
#include "log/log.h"

#include <chrono>

static bool g_motion_active = false;
static std::chrono::steady_clock::time_point g_last_motion_time;

// 움직임 감지 신호를 처리한다.
bool notify_motion_detected()
{
    const auto now = std::chrono::steady_clock::now();

    if (!g_motion_active) {
        if (!start_motion_event(get_buffer_segments())) {
            return false;
        }

        g_motion_active = true;

        log_write("cam", LogLevel::INFO, "motion detected");
    }

    g_last_motion_time = now;

    return true;
}

// motion 상태를 갱신한다.
bool update_motion_manager(const BufferUpdateResult& buffer_result)
{
    if (!g_motion_active) {
        return true;
    }

    if (buffer_result.segment_completed) {
        if (!add_motion_segment(buffer_result.completed_segment)) {
            return false;
        }
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(now - g_last_motion_time).count();

    if (elapsed_seconds < CAM_POST_MOTION_SECONDS) {
        return true;
    }

    BufferUpdateResult final_segment{};

    if (!force_finalize_current_segment(&final_segment)) {
        return false;
    }

    if (final_segment.segment_completed) {
        if (!add_motion_segment(final_segment.completed_segment)) {
            return false;
        }
    }

    if (!finish_motion_event()) {
        return false;
    }

    g_motion_active = false;

    return true;
}