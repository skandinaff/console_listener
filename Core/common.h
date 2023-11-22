#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <iostream>
#include <filesystem>

#include <thread>
#include <queue>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/bind/bind.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>


#ifdef IS_ARMBIAN
    static constexpr auto serial_port_name = "/dev/ttyS1";
#elif IS_RASPBIAN
    static constexpr auto serial_port_name = "/dev/serial0";
#endif

static constexpr auto baud_rate_number = 115200;

enum mState {
    IDLE,
    INIT,
    READ_CMD,
    HELP,
    EXIT
};

extern mState state;
extern mState prev_state;
extern std::mutex state_mutex;
extern std::condition_variable state_cv;
extern std::atomic<bool> exit_flag;
extern std::atomic<bool> data_ready;


/// @brief adds a path /home/<username> to a <filename>
/// @param filename 
/// @return "/home/<username>/<filename>"
inline std::string addHomePath(const std::string& filename) {
    std::filesystem::path executablePath = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::path baseDir = executablePath.parent_path().parent_path().parent_path();  // Adjust the parent_path() calls as needed
    std::filesystem::path configPath = baseDir / filename;
    return configPath.string();
}


#endif
