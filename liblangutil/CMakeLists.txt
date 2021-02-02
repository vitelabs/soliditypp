set(ORIGINAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/solidity/liblangutil)
set(sources
	${ORIGINAL_SOURCE_DIR}/Common.h
	${ORIGINAL_SOURCE_DIR}/CharStream.cpp
	${ORIGINAL_SOURCE_DIR}/CharStream.h
	${ORIGINAL_SOURCE_DIR}/ErrorReporter.cpp
	${ORIGINAL_SOURCE_DIR}/ErrorReporter.h
	${ORIGINAL_SOURCE_DIR}/EVMVersion.h
	${ORIGINAL_SOURCE_DIR}/EVMVersion.cpp
	${ORIGINAL_SOURCE_DIR}/Exceptions.cpp
	${ORIGINAL_SOURCE_DIR}/Exceptions.h
	${ORIGINAL_SOURCE_DIR}/ParserBase.cpp
	${ORIGINAL_SOURCE_DIR}/ParserBase.h
	${ORIGINAL_SOURCE_DIR}/Scanner.cpp
	${ORIGINAL_SOURCE_DIR}/Scanner.h
	${ORIGINAL_SOURCE_DIR}/SemVerHandler.cpp
	${ORIGINAL_SOURCE_DIR}/SemVerHandler.h
	${ORIGINAL_SOURCE_DIR}/SourceLocation.h
	${ORIGINAL_SOURCE_DIR}/SourceLocation.cpp
	${ORIGINAL_SOURCE_DIR}/SourceReferenceExtractor.cpp
	${ORIGINAL_SOURCE_DIR}/SourceReferenceExtractor.h
	${ORIGINAL_SOURCE_DIR}/SourceReferenceFormatter.cpp
	${ORIGINAL_SOURCE_DIR}/SourceReferenceFormatter.h
	${ORIGINAL_SOURCE_DIR}/Token.cpp
	Token.h
	${ORIGINAL_SOURCE_DIR}/UndefMacros.h
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_library(langutil ${sources})
set_target_properties(langutil PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(langutil PUBLIC solutil)
