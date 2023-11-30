#ifndef SERIAL_PORT_LISTENER_H
#define SERIAL_PORT_LISTENER_H

#include <boost/asio.hpp>
#include <sys/syscall.h>
#include "../include/common.h"
#include "../debug/include/debugUtils.h"

void serial_port_listener(mState& state,
                           boost::asio::serial_port& serial_port,
                           std::mutex& state_mutex,
                           std::condition_variable& state_cv,
                           std::queue<std::string>& data_queue,
                           std::atomic<bool>& exit_flag);

#endif // SERIAL_PORT_LISTENER_H