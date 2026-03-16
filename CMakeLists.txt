cmake_minimum_required(VERSION 3.16)

project(EZTrack)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tracker
    src/main.cpp
    src/core/Logger.cpp
    src/http/HttpClient.cpp
    src/notifications/NotificationManager.cpp
    src/tracker/ProductWatcher.cpp
    src/tracker/StockDetector.cpp
)

set_target_properties(tracker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)