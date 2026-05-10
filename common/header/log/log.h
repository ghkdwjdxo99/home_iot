#ifndef LOG_H
#define LOG_H

#include <string>
#include "log_config.h"

// 지정한 모듈의 날짜별 로그 파일에 로그를 기록한다.
// 예: logs/cam/2026-05-10.log
void log_write(const std::string& module_name,
               LogLevel level,
               const std::string& message);

#endif