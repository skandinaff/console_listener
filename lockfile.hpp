#ifndef LOCKFILE_H
#define LOCKFILE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

const std::string LOCKFILE_PATH = "bin/lockfile.lock";

bool lockfile_exists() {
    std::ifstream f(LOCKFILE_PATH);
    return f.good();
}

// Function to create lock file
void create_lockfile() {
    std::ofstream f(LOCKFILE_PATH);
    f.close();
}

// Function to remove lock file
void remove_lockfile() {
    remove(LOCKFILE_PATH.c_str());
}

void signal_handler(int signal_num) {
    if (signal_num == SIGINT || signal_num == SIGTERM || signal_num == SIGTSTP) {
        // Remove lockfile before terminating
        remove(LOCKFILE_PATH.c_str());
        exit(signal_num);
    }
}

#endif