set(solidity_test_base_sources
    # ${PROJECT_SOURCE_DIR}/solidity/test/boostTest.cpp
    Common.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/Common.h
    ${PROJECT_SOURCE_DIR}/solidity/test/CommonSyntaxTest.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/CommonSyntaxTest.h
    # ${PROJECT_SOURCE_DIR}/solidity/test/EVMHost.cpp
    # ${PROJECT_SOURCE_DIR}/solidity/test/EVMHost.h
    # ${PROJECT_SOURCE_DIR}/solidity/test/ExecutionFramework.cpp
    # ${PROJECT_SOURCE_DIR}/solidity/test/ExecutionFramework.h
    InteractiveTests.h
    ${PROJECT_SOURCE_DIR}/solidity/test/Metadata.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/Metadata.h
    TestCase.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/TestCase.h
    ${PROJECT_SOURCE_DIR}/solidity/test/TestCaseReader.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/TestCaseReader.h
)
set(sources
    main.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/ErrorCheck.cpp
    solidity/Scanner.cpp
    solidity/SolidityParser.cpp
    soliditypp/ScannerTest.cpp
    soliditypp/ParserTest.cpp
    libsolutil/Keccak256.cpp
    libsolutil/Blake2b.cpp
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

# creates the executable
add_executable(solpptest ${solidity_test_base_sources} ${sources})
target_link_libraries(solpptest PUBLIC Boost::boost Boost::filesystem Boost::program_options Boost::unit_test_framework langutil solidity)

# declares a test with test executable
add_test(NAME SolidityppTest COMMAND solpptest)

add_subdirectory(interactive)
add_subdirectory(evmc)