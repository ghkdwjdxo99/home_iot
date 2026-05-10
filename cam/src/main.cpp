#include "cam_config.h"

#include "buffer_manager.h"
#include "cam_init.h"
#include "log/log.h"
#include "motion_manager.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#if CAM_TEST_MODE == 1
#include <iostream>
#include <string>
#include <sys/select.h>
#include <unistd.h>
#endif

static std::atomic<bool> g_running(true);
static volatile std::sig_atomic_t g_received_signal = 0;

#if CAM_TEST_MODE == 1
static std::atomic<bool> g_test_motion_requested(false);
#endif

static void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        g_received_signal = signal;
        g_running = false;
    }
}

static void register_signal_handlers()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

static bool init_homecam_service()
{
    if (!init_homecam()) {
        log_write("cam", LogLevel::ERROR, "homecam initialization failed");
        return false;
    }

    if (!init_buffer_manager()) {
        log_write("cam", LogLevel::ERROR, "buffer manager initialization failed");
        return false;
    }

    log_write("cam", LogLevel::INFO, "homecam initialization success");

    return true;
}

#if CAM_TEST_MODE == 1

static bool is_stdin_ready()
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    const int result = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);

    return result > 0 && FD_ISSET(STDIN_FILENO, &read_fds);
}

static void input_thread_loop()
{
    log_write("cam", LogLevel::INFO, "test command ready: m=motion, q=quit");

    while (g_running) {
        if (!is_stdin_ready()) {
            continue;
        }

        std::string command;

        if (!std::getline(std::cin, command)) {
            log_write("cam", LogLevel::WARN, "test command input closed");
            return;
        }

        if (command == "m") {
            g_test_motion_requested = true;
        } else if (command == "q") {
            g_running = false;
        } else if (!command.empty()) {
            log_write("cam", LogLevel::WARN, "unknown test command: " + command);
        }
    }
}

#endif

static void sleep_main_loop()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(CAM_MAIN_LOOP_SLEEP_MS));
}

static void run_homecam_service()
{
    log_write("cam", LogLevel::INFO, "homecam main loop start");

#if CAM_TEST_MODE == 1
    std::thread input_thread(input_thread_loop);
#endif

    while (g_running) {
#if CAM_TEST_MODE == 1
        if (g_test_motion_requested.exchange(false)) {
            if (!notify_motion_detected()) {
                log_write("cam", LogLevel::ERROR, "failed to notify motion detected");
            }
        }
#endif

        BufferUpdateResult buffer_result{};

        if (!update_buffer_manager(&buffer_result)) {
            log_write("cam", LogLevel::ERROR, "buffer manager update failed");
        }

        if (!update_motion_manager(buffer_result)) {
            log_write("cam", LogLevel::ERROR, "motion manager update failed");
        }

        // motion_manager가 완료 segment를 복사한 뒤 오래된 buffer를 정리한다.
        if (!cleanup_buffer_segments()) {
            log_write("cam", LogLevel::ERROR, "buffer cleanup failed");
        }

        sleep_main_loop();
    }

#if CAM_TEST_MODE == 1
    if (input_thread.joinable()) {
        input_thread.join();
    }
#endif
}

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
    register_signal_handlers();

    log_write("cam", LogLevel::INFO, "homecam start");

    if (!init_homecam_service()) {
        return 1;
    }

    run_homecam_service();

    log_shutdown_reason();

    log_write("cam", LogLevel::INFO, "homecam exit");

    return 0;
}
