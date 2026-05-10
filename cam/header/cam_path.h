#ifndef CAM_PATH_H
#define CAM_PATH_H

#include <filesystem>

namespace cam_path {

namespace fs = std::filesystem;

// cam 기본 디렉터리
const fs::path CAM_DIR = "cam";

// cam runtime 디렉터리
const fs::path BUFFER_DIR = CAM_DIR / "buffer";
const fs::path CONFIG_DIR = CAM_DIR / "config";
const fs::path RECORDINGS_DIR = CAM_DIR / "recordings";

const fs::path RECORDINGS_PENDING_DIR = RECORDINGS_DIR / "pending";
const fs::path RECORDINGS_SYNCED_DIR = RECORDINGS_DIR / "synced";
const fs::path RECORDINGS_FAILED_DIR = RECORDINGS_DIR / "failed";

// 현재 작성 중인 segment 파일
const fs::path CURRENT_SEGMENT_TMP_FILE = BUFFER_DIR / "current_segment.tmp";

}

#endif
