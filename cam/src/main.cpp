#include <iostream>

#include "log/log.h"
#include "cam_init.h"

int main()
{
    // std::cout << "homecam start" << std::endl;
    log_write("cam", LogLevel::INFO, "homecam start");

    // homecam 실행에 필요한 초기 환경을 준비한다.
    if (!init_homecam()) {
        // std::cerr << "homecam initialization failed" << std::endl;
        log_write("cam", LogLevel::ERROR, "homecam initialization failed");
        return 1;
    }

    // std::cout << "homecam initialization success" << std::endl;
    // std::cout << "homecam exit" << std::endl;
    log_write("cam", LogLevel::INFO, "homecam start");

    return 0;
}