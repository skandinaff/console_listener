cmake_minimum_required(VERSION 3.2)
project(CL VERSION 1.0 LANGUAGES C CXX)
include(CheckSystem.cmake)

set(CMAKE_CXX_STANDARD 22)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
set(CMAKE_SYSTEM_NAME Linux)

set(SOURCES
        src/main.cpp
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
        serial 
        readline
        Threads::Threads
        ${Boost_LIBRARIES}
        )   

# Set the output directory for the executable
set_target_properties(CL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}usr/local/bin"
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

set(CPACK_GENERATOR "DEB")
#set(CPACK_PACKAGE_NAME "daccp")
set(CPACK_PACKAGE_NAME "CL")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "sevalks@gmail.com")

include(CPack)