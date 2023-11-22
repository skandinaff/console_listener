cmake_minimum_required(VERSION 3.0)
project(CL)
include(CheckSystem.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC")

set(SOURCES
        Core/main.cpp
        )

if (IS_ARMBIAN OR IS_RASPBIAN)
    message(STATUS "Configuring for system with serail port")
    list(APPEND SOURCES Core/serial_port_listener.cpp)
endif()

add_executable(CL ${SOURCES})

find_package(Threads REQUIRED)


if (IS_ARMBIAN OR IS_RASPBIAN)
    #target_compile_definitions(CL PRIVATE WITH_SERIAL)
    if(IS_ARMBIAN)
        target_compile_definitions(CL PRIVATE IS_ARMBIAN)
    elseif(IS_RASPBIAN)
        target_compile_definitions(CL PRIVATE IS_RASPBIAN)
    endif()
endif()




# Add the include directories
target_include_directories(CL PRIVATE 
        Threads::Threads
        )
target_link_libraries(CL PRIVATE
        readline
        )   

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
