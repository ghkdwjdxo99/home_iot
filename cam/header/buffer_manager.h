#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <filesystem>
#include <vector>

struct BufferUpdateResult {
    bool segment_completed;
    std::filesystem::path completed_segment;
};

bool init_buffer_manager();

bool update_buffer_manager(BufferUpdateResult* result);

bool force_finalize_current_segment(BufferUpdateResult* result);

std::vector<std::filesystem::path> get_buffer_segments();

// buffer에 남은 오래된 segment를 정리한다.
// motion_manager가 완료 segment를 처리한 뒤 호출해야 한다.
bool cleanup_buffer_segments();

#endif