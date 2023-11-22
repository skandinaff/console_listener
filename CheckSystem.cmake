file(STRINGS "/etc/os-release" OS_RELEASE_CONTENTS REGEX "PRETTY_NAME=")
string(REGEX REPLACE ".*PRETTY_NAME=\"?([^(\")]*)\"?.*" "\\1" OS_NAME "${OS_RELEASE_CONTENTS}")

if (OS_NAME MATCHES "Armbian")
    set(IS_ARMBIAN TRUE)
elseif (OS_NAME MATCHES "Raspbian")
    set(IS_RASPBIAN TRUE)
else()
    set(IS_DISPLAY TRUE)
endif()

message(STATUS "OS Name: ${OS_NAME}")
