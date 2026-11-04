cmake_minimum_required(VERSION 3.21)
project(qzapret VERSION 0.2.0 LANGUAGES CXX)

# Минимум — Qt 6.8 LTS (поддержка до 2029). API новее 6.8 не используем;
# при выходе дистрибутивных пакетов Qt 6.12 LTS проверяем сборку и на нём.
set(QZAPRET_QT_MIN 6.8 CACHE STRING "Minimum supported Qt version (LTS)")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 ${QZAPRET_QT_MIN} REQUIRED COMPONENTS Widgets LinguistTools)
qt_standard_project_setup(REQUIRES ${QZAPRET_QT_MIN})

set(QZAPRET_HELPER "${CMAKE_INSTALL_FULL_LIBEXECDIR}/qzapret/qzapret-helper")

set(QZAPRET_SOURCES
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

# Локализация. Исходные строки в коде русские; переводы — i18n/*.ts, вшиваются в ресурсы.
# Обновить .ts после правки строк: cmake --build build --target update_translations
qt_add_library(qzapret_i18n STATIC src/i18n.cpp src/i18n.h)
target_include_directories(qzapret_i18n PUBLIC src)
target_link_libraries(qzapret_i18n PUBLIC Qt6::Core)
qt_add_translations(qzapret_i18n
    TS_FILES i18n/qzapret_en.ts
    SOURCES ${QZAPRET_SOURCES} src/i18n.cpp
    INCLUDE_DIRECTORIES src
)

qt_add_executable(qzapret ${QZAPRET_SOURCES})
target_include_directories(qzapret PRIVATE src)
target_link_libraries(qzapret PRIVATE Qt6::Widgets qzapret_i18n)
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
    find_package(Qt6 ${QZAPRET_QT_MIN} REQUIRED COMPONENTS Test)
    qt_add_executable(test_blockcheck tests/test_blockcheck.cpp
        src/backend/blockcheckrunner.cpp src/backend/blockcheckrunner.h)
    target_include_directories(test_blockcheck PRIVATE src)
    target_link_libraries(test_blockcheck PRIVATE Qt6::Test Qt6::Core)
    target_compile_definitions(test_blockcheck PRIVATE QZAPRET_HELPER_PATH="/nonexistent")
    add_test(NAME blockcheck COMMAND test_blockcheck)

    qt_add_executable(test_i18n tests/test_i18n.cpp)
    target_link_libraries(test_i18n PRIVATE Qt6::Test qzapret_i18n)
    target_compile_definitions(test_i18n PRIVATE QZAPRET_I18N_DIR="${CMAKE_SOURCE_DIR}/i18n")
    add_test(NAME i18n COMMAND test_i18n)

    add_test(NAME helper_validation
             COMMAND sh ${CMAKE_SOURCE_DIR}/tests/test_helper.sh
                     ${CMAKE_SOURCE_DIR}/packaging/helper/qzapret-helper)
endif()
