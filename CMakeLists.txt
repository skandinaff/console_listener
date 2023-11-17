cmake_minimum_required(VERSION 3.0)
project(CL)
# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC")

# Add the source files for your image processing program
add_executable(CL
        main.cpp
        )

#include_directories(/usr/include/libserial /usr/include/jsoncpp)
find_package(Threads REQUIRED)

# Add the include directories
target_include_directories(CL PRIVATE 
        Threads::Threads
        )

#target_link_libraries(CL jpeg serial jsoncpp)

# Set the output directory for the executable
set_target_properties(CL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET CL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_SOURCE_DIR}/bin/lockfile.lock"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(CL PRIVATE DEBUG)
    target_compile_options(CL PRIVATE -g)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(CL PRIVATE -O3)
endif()
