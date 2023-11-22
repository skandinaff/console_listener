#include <iostream>

#include "common.h"
#include "lockfile.hpp"
#include "serial_port_listener.h"
#include "../debug/include/debugUtils.h"
#include <termios.h>

struct termios orig_termios;

using namespace boost::asio;

mState state = IDLE;
mState prev_state = IDLE;
std::mutex state_mutex;
std::condition_variable state_cv;
std::atomic<bool> exit_flag(false);
std::atomic<bool> data_ready(false);

std::unordered_map<std::string, std::pair<mState, std::string>> command_map = {
        {"idle", {IDLE, "Idle state"}},
        {"init", {INIT, "Initialize"}},
        {"exit", {EXIT, "Exit app"}},
        {"help", {HELP, "Show this list"}}
};
void reset_terminal() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) == -1) {
        perror("tcsetattr");
    }
    rl_cleanup_after_signal();
}
void check_arguments(int argc, char* argv[]){

    if (argc > 1 && std::string(argv[1]) == "-I") {
        std::cout << "arg: I" << std::endl;
        state = INIT;
    } else if (argc > 1 && std::string(argv[1]) == "-h") {
        std::cout << "arg: h" << std::endl;
        state = HELP;
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

void console_input_listener(mState& state,
                        std::mutex& state_mutex, 
                        std::condition_variable& state_cv, 
                        std::queue<std::string>& data_queue,
                        std::atomic<bool>& exit_flag
                        )
{
    const char* prompt = "\033[1;32m$$\033[0m ";    
    pid_t tid = syscall(SYS_gettid); 

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    INFO("Console listener Thread LWP ID: " << tid);
    while (!exit_flag) {
        if(exit_flag) break;
        char* input = nullptr;
        input = readline(prompt);

        if (input == nullptr || std::cin.eof()) {
            break;
        }
        add_history(input);

        std::string cmd(input);
        free(input);
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            data_queue.push(cmd);
            state_cv.notify_all();
            if (input == "exit") {
                state = EXIT;
                break;
            }
        }
    }
    INFO( "Left console input..."  );
}

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
        std::cout << "Unknown command. ";
        state = HELP;
    }
    //INFO("State changed from " << prev_state << " to " << state );
    prev_state = state;
    //INFO("Left cmd decoder..." );
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
    pid_t tid = syscall(SYS_gettid); 
    INFO( "State Machine Thread LWP ID: " << tid );
    bool idle_msg_printed = false;
    bool init_done = false;

    //while (state != EXIT) {
    while(!exit_flag){
        switch (state) {
            case IDLE: {
                /*
                if (!idle_msg_printed) {
                    std::cout << "In IDLE state, waiting for commands:\n";
                    idle_msg_printed = true;
                }*/
                std::lock_guard<std::mutex> lock(state_mutex);
                if (!data_queue.empty()) {
                    state = mState::READ_CMD;
                    break;
                }
                break;
            }
            case INIT: {
                INFO("In INIT state" );
                if(!init_done){
                    // Do the init
                    init_done = true;
                }
                state = mState::IDLE;
                break;
            }
            case READ_CMD: {
                //INFO("In READ_CMD state" );
                std::string cmd;
                {
                    std::lock_guard<std::mutex> lock(state_mutex);
                    cmd = data_queue.front();
                    data_queue.pop();
                }
                //idle_msg_printed = false;
                //INFO("Received command: " << cmd);
                decode_cmd(cmd, state, prev_state, state_cv);
                break;
            }      
            case HELP: {
                std::cout << "List of avaliable commands:\n" << get_available_commands(command_map) << std::endl;
                state = mState::IDLE;
                break;
            }
            case EXIT: {
                INFO("Entered Exit State");
#ifndef DEBUG
                remove_lockfile();
#endif
                {
                    std::lock_guard<std::mutex> lock(state_mutex);
                    exit_flag = true;
                    state_cv.notify_all();
                }
                break;
            }
            default: {
                std::cout << "Invalid state" << std::endl;
                break;
            }
        }
    }
    INFO("Left state machine...");
    reset_terminal();
    exit(0); // TODO: resolve thread exit when with serial thread.
}


int main(int argc, char* argv[])
{
    INFO("Starting program...");

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }

    check_arguments(argc, argv);
#ifndef DEBUG
    if (lockfile_exists()) {
        std::cerr << "Error: Another instance of the program is already running." << std::endl;
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
    boost::asio::write(serial_port, boost::asio::buffer("CL app started! State: Idle"));
    
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

    

    INFO("Program finished.");

    return 0;
}


