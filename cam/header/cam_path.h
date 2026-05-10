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

// buffer 파일
const fs::path PREV_BUFFER_FILE = BUFFER_DIR / "prev.txt";
const fs::path CURRENT_BUFFER_FILE = BUFFER_DIR / "current.txt";
const fs::path CURRENT_BUFFER_TMP_FILE = BUFFER_DIR / "current.tmp";

}

#endif
