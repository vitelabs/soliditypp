set(solidity_test_base_sources
    boostTest.cpp
    Common.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/Common.h
    ${PROJECT_SOURCE_DIR}/solidity/test/CommonSyntaxTest.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/CommonSyntaxTest.h
    ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/SyntaxTest.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/SyntaxTest.h
    ${PROJECT_SOURCE_DIR}/solidity/test/EVMHost.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/EVMHost.h
    ${PROJECT_SOURCE_DIR}/solidity/test/ExecutionFramework.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/ExecutionFramework.h
    InteractiveTests.h
    ${PROJECT_SOURCE_DIR}/solidity/test/Metadata.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/Metadata.h
    TestCase.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/TestCase.h
    ${PROJECT_SOURCE_DIR}/solidity/test/TestCaseReader.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/TestCaseReader.h
    libsolidity/AnalysisFramework.cpp
    libsolidity/AnalysisFramework.h
)
set(sources
    # main.cpp
    ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/ErrorCheck.cpp
    libsolidity/SolidityTypes.cpp
    libsolidity/AST.cpp
    libsolidity/SolidityExpressionCompiler.cpp
    libsolidity/SolidityNameAndTypeResolution.cpp
    solidity/Scanner.cpp
    solidity/SolidityParser.cpp
    soliditypp/ScannerTest.cpp
    soliditypp/ParserTest.cpp
    soliditypp/SolidityppExpressionCompiler.cpp
    soliditypp/SolidityppNameAndTypeResolution.cpp
    libsolutil/Keccak256.cpp
    libsolutil/Blake2b.cpp
    libsolutil/CommonData.cpp
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

# creates the executable
add_executable(solpptest ${solidity_test_base_sources} ${sources})
target_link_libraries(solpptest PRIVATE langutil yul solidity smtutil solutil evmasm Boost::boost Boost::filesystem Boost::program_options Boost::unit_test_framework evmc)

# declares a test with test executable
# add_test(NAME SolidityppTest COMMAND solpptest)

add_subdirectory(interactive)
add_subdirectory(evmc)