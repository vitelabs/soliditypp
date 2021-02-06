set(sources
	isoltest.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/tools/IsolTestOptions.cpp
	../Common.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/CommonSyntaxTest.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/EVMHost.cpp
	../TestCase.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/TestCaseReader.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/util/BytesUtils.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/util/ContractABIUtils.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/util/TestFileParser.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/util/TestFunctionCall.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/GasTest.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/SyntaxTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/SemanticTest.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/AnalysisFramework.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/SolidityExecutionFramework.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/ExecutionFramework.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/ABIJsonTest.cpp
	${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/ASTJSONTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libsolidity/SMTCheckerTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/Common.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/EwasmTranslationTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/FunctionSideEffects.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/ObjectCompilerTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/SyntaxTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/YulOptimizerTest.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/YulOptimizerTestCommon.cpp
	# ${PROJECT_SOURCE_DIR}/solidity/test/libyul/YulInterpreterTest.cpp
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_executable(itest ${sources})

target_link_libraries(itest PRIVATE evmc solidity evmasm Boost::boost Boost::program_options Boost::unit_test_framework)