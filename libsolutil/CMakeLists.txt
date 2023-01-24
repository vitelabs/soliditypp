set(ORIGINAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/solidity/libsolutil)

set(sources
	Algorithms.h
	${ORIGINAL_SOURCE_DIR}/AnsiColorized.h
	${ORIGINAL_SOURCE_DIR}/Assertions.h
	${ORIGINAL_SOURCE_DIR}/Common.cpp
	${ORIGINAL_SOURCE_DIR}/Common.h
	CommonData.cpp
	CommonData.h
	${ORIGINAL_SOURCE_DIR}/CommonIO.cpp
	${ORIGINAL_SOURCE_DIR}/CommonIO.h
	${ORIGINAL_SOURCE_DIR}/cxx20.h
	${ORIGINAL_SOURCE_DIR}/Exceptions.cpp
	${ORIGINAL_SOURCE_DIR}/Exceptions.h
	${ORIGINAL_SOURCE_DIR}/ErrorCodes.h
	FixedHash.h
	FunctionSelector.h
	${ORIGINAL_SOURCE_DIR}/IndentedWriter.cpp
	${ORIGINAL_SOURCE_DIR}/IndentedWriter.h
	${ORIGINAL_SOURCE_DIR}/IpfsHash.cpp
	${ORIGINAL_SOURCE_DIR}/IpfsHash.h
	${ORIGINAL_SOURCE_DIR}/JSON.cpp
	${ORIGINAL_SOURCE_DIR}/JSON.h
	${ORIGINAL_SOURCE_DIR}/Keccak256.cpp
	${ORIGINAL_SOURCE_DIR}/Keccak256.h
	${ORIGINAL_SOURCE_DIR}/LazyInit.h
	${ORIGINAL_SOURCE_DIR}/LEB128.h
	${ORIGINAL_SOURCE_DIR}/picosha2.h
	${ORIGINAL_SOURCE_DIR}/Result.h
	${ORIGINAL_SOURCE_DIR}/SetOnce.h
	${ORIGINAL_SOURCE_DIR}/StringUtils.cpp
	${ORIGINAL_SOURCE_DIR}/StringUtils.h
	${ORIGINAL_SOURCE_DIR}/SwarmHash.cpp
	${ORIGINAL_SOURCE_DIR}/SwarmHash.h
	${ORIGINAL_SOURCE_DIR}/UTF8.cpp
	${ORIGINAL_SOURCE_DIR}/UTF8.h
	${ORIGINAL_SOURCE_DIR}/vector_ref.h
	${ORIGINAL_SOURCE_DIR}/Views.h
	${ORIGINAL_SOURCE_DIR}/Visitor.h
	${ORIGINAL_SOURCE_DIR}/Whiskers.cpp
	${ORIGINAL_SOURCE_DIR}/Whiskers.h
	# Solidity++
	Blake2.h
	Blake2Impl.h
	Blake2bRef.cpp
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_library(solutil ${sources})
target_link_libraries(solutil PUBLIC jsoncpp Boost::boost Boost::filesystem Boost::system range-v3)
target_include_directories(solutil PUBLIC "${CMAKE_SOURCE_DIR}")
# add_dependencies(solutil solidity_BuildInfo.h)

if(SOLC_LINK_STATIC)
	target_link_libraries(solutil PUBLIC Threads::Threads)
endif()
