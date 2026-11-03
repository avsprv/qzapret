cmake_minimum_required(VERSION 3.21)
project(qzapret VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(QZAPRET_HELPER "${CMAKE_INSTALL_FULL_LIBEXECDIR}/qzapret/qzapret-helper")

qt_add_executable(qzapret
    src/main.cpp
    src/autostart.cpp
    src/autostart.h
    src/blockcheckpage.cpp
    src/blockcheckpage.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/backend/blockcheckrunner.cpp
    src/backend/blockcheckrunner.h
    src/backend/helperpath.h
    src/backend/systemdbackend.cpp
    src/backend/systemdbackend.h
    src/backend/zapretbackend.h
)
target_include_directories(qzapret PRIVATE src)
target_link_libraries(qzapret PRIVATE Qt6::Widgets)
target_compile_definitions(qzapret PRIVATE
    QZAPRET_VERSION="${PROJECT_VERSION}"
    QZAPRET_HELPER_PATH="${QZAPRET_HELPER}")
target_compile_options(qzapret PRIVATE -Wall -Wextra)

configure_file(data/org.qzapret.policy.in ${CMAKE_BINARY_DIR}/org.qzapret.policy @ONLY)

install(TARGETS qzapret RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS packaging/helper/qzapret-helper
        DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/qzapret)
install(FILES ${CMAKE_BINARY_DIR}/org.qzapret.policy
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions)
install(FILES data/qzapret.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(test_blockcheck tests/test_blockcheck.cpp
        src/backend/blockcheckrunner.cpp src/backend/blockcheckrunner.h)
    target_include_directories(test_blockcheck PRIVATE src)
    target_link_libraries(test_blockcheck PRIVATE Qt6::Test Qt6::Core)
    target_compile_definitions(test_blockcheck PRIVATE QZAPRET_HELPER_PATH="/nonexistent")
    add_test(NAME blockcheck COMMAND test_blockcheck)

    add_test(NAME helper_validation
             COMMAND sh ${CMAKE_SOURCE_DIR}/tests/test_helper.sh
                     ${CMAKE_SOURCE_DIR}/packaging/helper/qzapret-helper)
endif()
