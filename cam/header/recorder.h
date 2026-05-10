#ifndef RECORDER_H
#define RECORDER_H

#include <filesystem>
#include <vector>

// motion event 임시 디렉터리를 생성하고 pre-buffer segment들을 복사한다.
bool start_motion_event(const std::vector<std::filesystem::path>& pre_segments);

// motion 중 새로 완료된 segment를 event 디렉터리에 추가한다.
bool add_motion_segment(const std::filesystem::path& segment_path);

// motion event를 확정한다.
bool finish_motion_event();

#endif