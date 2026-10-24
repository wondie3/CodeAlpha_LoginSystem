cmake_minimum_required(VERSION 3.16)
project(CodeAlpha_LoginSystem_GUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Accounts, password hashing, storage and CSV export. No Qt code, so it is easy to test.
add_library(login_core STATIC
    core/UserStore.cpp  core/UserStore.h
    core/Crypto.cpp     core/Crypto.h
    core/Csv.cpp        core/Csv.h
    core/FileUtil.cpp   core/FileUtil.h
)
target_include_directories(login_core PUBLIC core)

# The Qt 6 Widgets application
add_executable(LoginGUI WIN32 MACOSX_BUNDLE
    gui/main.cpp
    gui/MainWindow.cpp  gui/MainWindow.h
    gui/AuthPage.cpp    gui/AuthPage.h
    gui/Dialogs.cpp     gui/Dialogs.h
    gui/Widgets.cpp     gui/Widgets.h
    gui/Style.h
)
target_link_libraries(LoginGUI PRIVATE login_core Qt6::Widgets)

# Unit tests for the core (no GUI needed)
option(BUILD_TESTS "Build the unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_core tests/test_core.cpp)
    target_link_libraries(test_core PRIVATE login_core)
    add_test(NAME core COMMAND test_core)
endif()
