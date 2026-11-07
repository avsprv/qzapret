    qt_add_executable(test_profileanalyzer tests/test_profileanalyzer.cpp
        src/profileanalyzer.cpp src/profileanalyzer.h)
    target_include_directories(test_profileanalyzer PRIVATE src)
    target_link_libraries(test_profileanalyzer PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME profile_analyzer COMMAND test_profileanalyzer)
