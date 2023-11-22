#include <iostream>
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

#include "common.h"
#include "lockfile.hpp"



using namespace std;
using namespace boost::asio;

std::unordered_map<std::string, std::pair<mState, std::string>> command_map = {
        {"idle", {IDLE, "Idle state"}},
        {"init", {INIT, "Initialize"}},
        {"exit", {EXIT, "Exit app"}},
        {"help", {HELP, "Show this list"}}
};

mState state = IDLE;
mState prev_state = IDLE;
std::mutex state_mutex;
std::condition_variable state_cv;
std::atomic<bool> exit_flag(false);
std::atomic<bool> data_ready(false);

void check_arguments(int argc, char* argv[]){

    if (argc > 1 && string(argv[1]) == "-I") {
        cout << "arg: I" << endl;
        state = mState::INIT;
    } else if (argc > 1 && string(argv[1]) == "-h") {
        cout << "arg: h" << endl;
        state = mState::HELP;
    }
}

std::string get_available_commands(const std::unordered_map<std::string, std::pair<mState, std::string>>& command_map) {
    std::ostringstream oss;
    oss << "\n";
    for (const auto& entry : command_map) {
        oss << entry.first << ": " << entry.second.second << "\n";
    }
    std::string result = oss.str();
    if (!result.empty()) {
        result = result.substr(0, result.size() - 1);  // Remove the trailing comma and space
    }
    return result;
}

void serial_port_listener(mState& state, 
                        boost::asio::serial_port& serial_port, 
                        std::mutex& state_mutex,
                        std::condition_variable& state_cv, 
                        std::queue<std::string>& data_queue,
                        std::atomic<bool>& exit_flag
                        )
{
    pid_t tid = syscall(SYS_gettid); std::cout << "Serial Port Thread LWP ID: " << tid << std::endl;
    //DEBUG_PRINT("Serial Port Thread LWP ID: ", tid);
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
            cout << "serial intput received: " << line << endl;
            if(line == "exit") break;
        }
    }
    cout << "Left serial listener..."  << endl;
    serial_port.close();
}

void console_input_listener(mState& state,
                        std::mutex& state_mutex, 
                        std::condition_variable& state_cv, 
                        std::queue<std::string>& data_queue,
                        std::atomic<bool>& exit_flag
                        )
{
    std::string input;
    pid_t tid = syscall(SYS_gettid); 
    //std::cout << "Console listener Thread LWP ID: " << tid << std::endl;
    //DEBUG_PRINT("Console listener Thread LWP ID: ", tid);
    while (!exit_flag.load()) {
        if(exit_flag) break;
        if (!std::getline(std::cin, input) || std::cin.eof()) {
            break;
        }
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            data_queue.push(input);
            state_cv.notify_all();
            if (input == "exit") {
                state = EXIT;
                break;
            }
        }
    }
    cout << "Left console input..."  << endl;
}
/**/
void decode_cmd(std::string cmd, mState& state, mState& prev_state, std::condition_variable& state_cv){
    if(cmd.empty()){
        state = IDLE;
        return;
    }
    std::istringstream iss(cmd);
    std::vector<std::string> cmd_tokens(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
    auto iter = command_map.find(cmd_tokens[0]);
    if (iter != command_map.end()) {
        state = iter->second.first;
    }   
    else {
        std::cout << "Unknown command. " << get_available_commands(command_map) << std::endl;
        state = IDLE;
    }
    std::cout << "State changed from " << prev_state << " to " << state << std::endl;
    prev_state = state;
    std::cout << "Left cmd decoder..." << std::endl;
}

void state_machine(mState& state, 
                   mState& prev_state, 
                   boost::asio::serial_port& serial_port, 
                   std::mutex& state_mutex, 
                   std::condition_variable& state_cv, 
                   std::queue<std::string>& data_queue, 
                   std::atomic<bool>& exit_flag,
                   std::atomic<bool>& data_ready
                   )
{
    pid_t tid = syscall(SYS_gettid); std::cout << "State Machine Thread LWP ID: " << tid << std::endl;
    int files_in_folder = 0;
    bool idle_msg_printed = false;
    bool auto_process = false;
    bool init_done = false;
    std::chrono::steady_clock::time_point start, interlude, stop; 
    std::chrono::milliseconds duration_ms; 
    std::chrono::microseconds duration_us;

    while (state != EXIT) {
        switch (state) {
            case IDLE: {
                if (!idle_msg_printed) {
                    std::cout << "In IDLE state, waiting for commands:" << std::endl;
                    idle_msg_printed = true;
                }
                std::lock_guard<std::mutex> lock(state_mutex);
                if (!data_queue.empty()) {
                    state = mState::READ_CMD;
                    break;
                }
                break;
            }
            case INIT: {
                std::cout << "In INIT state" << std::endl;
                if(!init_done){
                    // Do the init
                    init_done = true;
                }
                state = mState::IDLE;
                break;
            }
            case READ_CMD: {
                std::cout << "In READ_CMD state" << std::endl;
                std::string cmd;
                {
                    std::lock_guard<std::mutex> lock(state_mutex);
                    cmd = data_queue.front();
                    data_queue.pop();
                }
                idle_msg_printed = false;
                std::cout << "Received command: " << cmd << std::endl;
                decode_cmd(cmd, state, prev_state, state_cv);
                break;
            }      
            case HELP: {
                std::cout << "List of avaliable commands:\n" << get_available_commands(command_map) << std::endl;
                state = mState::IDLE;
                break;
            }
            case EXIT: {
                std::cout << "Entered Exit State" << std::endl;
#ifndef DEBUG
                remove_lockfile();
#endif
                {
                    std::lock_guard<std::mutex> lock(state_mutex);
                    exit_flag = true;
                }
                break;
            }
            default: {
                std::cout << "Invalid state" << std::endl;
                break;
            }
        }
    }
    cout << "Left state machine..."  << endl;
    exit(0); // TODO: resolve three thread exit.
}


int main(int argc, char* argv[])
{
    cout << "Starting program..." << endl;
    check_arguments(argc, argv);
#ifndef DEBUG
    if (lockfile_exists()) {
        cerr << "Error: Another instance of the program is already running." << endl;
        return 1;
    }
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGTSTP, signal_handler);
    atexit(remove_lockfile);
    // Create lock file
    create_lockfile();
#endif
    std::queue<std::string> data_queue;
    boost::asio::io_context serial_io_context;
    boost::asio::serial_port serial_port(serial_io_context);
    // Start threads
    std::thread state_machine_thread(state_machine, 
                                    std::ref(state),
                                    std::ref(prev_state), 
                                    std::ref(serial_port), 
                                    std::ref(state_mutex), 
                                    std::ref(state_cv), 
                                    std::ref(data_queue),
                                    std::ref(exit_flag),
                                    std::ref(data_ready)
                                    );
    std::thread console_thread(console_input_listener, 
                                    std::ref(state), 
                                    std::ref(state_mutex), 
                                    std::ref(state_cv), 
                                    std::ref(data_queue),
                                    std::ref(exit_flag)
                                    );    
#ifdef WITH_SERIAL
    serial_port.open(serial_port_name);
    serial_port.set_option(boost::asio::serial_port_base::baud_rate(baud_rate_number));
    boost::asio::write(serial_port, boost::asio::buffer("CamProc started! State: Idle"));
    std::thread serial_thread(serial_port_listener, 
                                    std::ref(state), 
                                    std::ref(serial_port), 
                                    std::ref(state_mutex), 
                                    std::ref(state_cv), 
                                    std::ref(data_queue),
                                    std::ref(exit_flag)
                                    );
#endif


#ifdef WITH_SERIAL
    serial_thread.join();
#endif

    state_machine_thread.join();
    console_thread.join();


    
    cout << "Program finished." << endl;

    return 0;
}


