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
#include <boost/bind/bind.hpp>

#include <readline/readline.h>
#include <readline/history.h>

#ifdef IS_ARMBIAN
    static constexpr auto serial_port_name = "/dev/ttyS1";
#elif IS_RASPBIAN
    static constexpr auto serial_port_name = "/dev/serial0";
#endif

static constexpr auto baud_rate_number = 115200;

static const char* prompt = "\033[1;32m$$\033[0m ";    

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

#endif
