set(sources
	Common.h
	CharStream.cpp
	CharStream.h
	ErrorReporter.cpp
	ErrorReporter.h
	EVMVersion.h
	EVMVersion.cpp
	Exceptions.cpp
	Exceptions.h
	ParserBase.cpp
	ParserBase.h
	Scanner.cpp
	Scanner.h
	SemVerHandler.cpp
	SemVerHandler.h
	SourceLocation.h
	SourceLocation.cpp
	SourceReferenceExtractor.cpp
	SourceReferenceExtractor.h
	SourceReferenceFormatter.cpp
	SourceReferenceFormatter.h
	Token.cpp
	Token.h
	UndefMacros.h
)

# Set sources as My.h -> ${PROJECT_SOURCE_DIR}/solidity/lib/My.h
set(DIR ${PROJECT_SOURCE_DIR}/solidity/liblangutil)
list (TRANSFORM sources PREPEND "${DIR}/")

include_directories(${PROJECT_SOURCE_DIR}/solidity)

add_library(langutil ${sources})
set_target_properties(langutil PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(langutil PUBLIC solutil)
