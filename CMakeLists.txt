cmake_minimum_required(VERSION 3.16)

project(EZTrack)

set(CMAKE_CXX_STANDARD 17)

add_executable(tracker
    src/main.cpp
    src/tracker/ProductWatcher.cpp
    src/http/HttpClient.cpp
    src/notifications/NotificationManager.cpp
)

# Put executables directly in build/
set_target_properties(tracker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)