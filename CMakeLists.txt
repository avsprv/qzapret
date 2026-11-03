cmake_minimum_required(VERSION 3.21)
project(qzapret VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(qzapret
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/backend/zapretbackend.h
    src/backend/systemdbackend.cpp
    src/backend/systemdbackend.h
)
target_include_directories(qzapret PRIVATE src)
target_link_libraries(qzapret PRIVATE Qt6::Widgets)
target_compile_definitions(qzapret PRIVATE QZAPRET_VERSION="${PROJECT_VERSION}")
target_compile_options(qzapret PRIVATE -Wall -Wextra)

install(TARGETS qzapret RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/qzapret.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
