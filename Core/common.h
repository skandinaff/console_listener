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

#include <readline/readline.h>
#include <readline/history.h>

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

#endif
