include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

# creates the executable
add_executable(solpptest ParserTest.cpp)
target_link_libraries(solpptest PUBLIC Boost::boost Boost::unit_test_framework langutil solidity)

# declares a test with test executable
add_test(NAME ParserTest COMMAND solpptest)