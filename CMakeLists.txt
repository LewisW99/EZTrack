cmake_minimum_required(VERSION 3.16)

project(EZTrack)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------
# Executable
# ------------------------------------------------

add_executable(tracker)

# ------------------------------------------------
# Source files
# ------------------------------------------------

target_sources(tracker PRIVATE

    src/main.cpp

    src/core/Logger.cpp

    src/http/HttpClient.cpp

    src/tracker/ProductWatcher.cpp
    src/tracker/StockDetector.cpp

    src/notifications/NotificationManager.cpp

    src/database/Database.cpp

    external/sqlite/sqlite3.c
)

# ------------------------------------------------
# Include directories
# ------------------------------------------------

target_include_directories(tracker PRIVATE

    src
    src/core
    src/http
    src/tracker
    src/notifications
    src/database

    external
    external/sqlite
)

# ------------------------------------------------
# Platform specific settings
# ------------------------------------------------

if(WIN32)

    target_compile_definitions(tracker PRIVATE _CRT_SECURE_NO_WARNINGS)

endif()

# ------------------------------------------------
# Threads (Linux / Pi)
# ------------------------------------------------

find_package(Threads REQUIRED)
target_link_libraries(tracker PRIVATE Threads::Threads)

# ------------------------------------------------
# Output directory
# ------------------------------------------------

set_target_properties(tracker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)