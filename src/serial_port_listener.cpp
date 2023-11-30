#include "serial_port_listener.h"

void serial_port_listener(mState& state, 
                        boost::asio::serial_port& serial_port, 
                        std::mutex& state_mutex,
                        std::condition_variable& state_cv, 
                        std::queue<std::string>& data_queue,
                        std::atomic<bool>& exit_flag
                        )
{
    pid_t tid = syscall(SYS_gettid);
    INFO("Serial Port Thread LWP ID: " << tid);
    while (!exit_flag.load()) {
        if(exit_flag) break;
        std::string line;
        boost::asio::read_until(serial_port, boost::asio::dynamic_buffer(line), '\n');
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            data_queue.push(line);
            state_cv.notify_all();
            INFO("serial intput received: " );
            if(line == "exit") break;
        }
    }
    INFO("Left serial listener...");
    serial_port.close();
}