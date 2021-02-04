set(sources
    main.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/ErrorCheck.cpp
    solidity/Scanner.cpp
    solidity/SolidityParser.cpp
    soliditypp/ScannerTest.cpp
    soliditypp/ParserTest.cpp
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

# creates the executable
add_executable(solpptest ${sources})
target_link_libraries(solpptest PUBLIC Boost::boost Boost::unit_test_framework langutil solidity)

# declares a test with test executable
add_test(NAME ParserTest COMMAND solpptest)