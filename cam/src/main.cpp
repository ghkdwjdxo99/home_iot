#include "cam_init.h"
#include "buffer_manager.h"
#include "log/log.h"

#include <atomic>
#include <csignal>
#include <chrono>
#include <thread>

// 프로그램이 계속 실행 중인지 나타내는 전역 상태 변수
static std::atomic<bool> g_running(true);

// 어떤 종료 신호를 받았는지 저장
static volatile std::sig_atomic_t g_received_signal = 0;

// 종료 신호를 받으면 main loop를 빠져나가기 위해 running 상태를 false로 바꾼다.
static void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        g_received_signal = signal;
        g_running = false;
    }
}

// Ctrl+C 또는 종료 요청을 처리하기 위한 signal handler를 등록한다.
static void register_signal_handlers()
{
    // interrupt signal
    // 터미널 실행 중 Ctrl + C 입력 시 발생
    std::signal(SIGINT, signal_handler);

    // termination signal
    // kill <PID> 또는 systemd stop 등으로 발생
    std::signal(SIGTERM, signal_handler);
}

// homecam 서비스 실행에 필요한 초기화를 수행한다.
static bool init_homecam_service()
{
    // homecam 실행에 필요한 디렉터리를 준비한다.
    if (!init_homecam()) {
        log_write("cam", LogLevel::ERROR, "homecam initialization failed");
        return false;
    }

    log_write("cam", LogLevel::INFO, "homecam initialization success");

    // buffer manager를 초기화한다.
    if (!init_buffer_manager()) {
        log_write("cam", LogLevel::ERROR, "buffer manager initialization failed");
        return false;
    }

    log_write("cam", LogLevel::INFO, "buffer manager initialization success");

    return true;
}

// 종료 신호가 들어오면 짧은 주기로 확인하면서 대기한다.
static void sleep_with_stop_check()
{
    for (int i = 0; i < 50 && g_running; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// homecam 서비스의 실제 실행 loop를 수행한다.
static void run_homecam_service()
{
    log_write("cam", LogLevel::INFO, "homecam main loop start");

    while (g_running) {
        log_write("cam", LogLevel::INFO, "homecam alive");

        if (!rotate_buffer_file()) {
            log_write("cam", LogLevel::ERROR, "buffer rotate failed");
        }

        sleep_with_stop_check();
    }
}

// 종료 요청 종류에 따라 종료 로그를 출력한다.
static void log_shutdown_reason()
{
    if (g_received_signal == SIGINT) {
        log_write("cam", LogLevel::INFO, "homecam stop requested");
    } else if (g_received_signal == SIGTERM) {
        log_write("cam", LogLevel::INFO, "homecam termination requested");
    }
}

int main()
{
    // Ctrl+C 또는 종료 요청을 처리하기 위한 signal handler를 등록한다.
    register_signal_handlers();

    log_write("cam", LogLevel::INFO, "homecam start");

    // homecam 서비스 실행에 필요한 초기화를 수행한다.
    if (!init_homecam_service()) {
        return 1;
    }

    // 실제 동작 loop
    run_homecam_service();

    // 종료 요청 종류에 따라 종료 로그를 출력한다.
    log_shutdown_reason();

    log_write("cam", LogLevel::INFO, "homecam exit");

    return 0;
}