cmake_minimum_required(VERSION 3.10)
project(keybinder)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find libevdev
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

# Include directories for libevdev
include_directories(${LIBEVDEV_INCLUDE_DIRS})

# Link directories for libevdev
link_directories(${LIBEVDEV_LIBRARY_DIRS})

# Add compiler flags for libevdev (if needed)
add_definitions(${LIBEVDEV_CFLAGS_OTHER})

# Add the executable
add_executable(keybinder main.cpp
        ProfileManager.cpp
        ProfileManager.h
)

# Link libevdev to your executable
target_link_libraries(keybinder ${LIBEVDEV_LIBRARIES})