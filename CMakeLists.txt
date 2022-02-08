cmake_minimum_required(VERSION 3.13.0)

set(ETH_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/solidity/cmake"   CACHE PATH "The the path to the solidity cmake directory")
set(VITE_CMAKE_DIR   "${CMAKE_CURRENT_LIST_DIR}/cmake"   CACHE PATH "The the path to the soliditypp cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR} ${VITE_CMAKE_DIR})

project(solidity VERSION 0.8.0 LANGUAGES C CXX)

enable_testing()

# Set the build type, if none was specified.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
		set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
	else()
		set(DEFAULT_BUILD_TYPE "Release")
	endif()
	set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

include(ViteUtils)

include(EthToolchains)

option(SOLC_LINK_STATIC "Link solc executable statically on supported platforms" ON)
option(SOLC_STATIC_STDLIBS "Link solc against static versions of libgcc and libstdc++ on supported platforms" OFF)

# Let's find our dependencies
include(EthDependencies)
include(jsoncpp)
include(range-v3)
include_directories(SYSTEM ${JSONCPP_INCLUDE_DIR})

find_package(Threads)

include(EthPolicy)
eth_policy()

set(PROJECT_VERSION "0.8.0")

# Figure out what compiler and system are we using
include(ViteCompilerSettings)

# Include utils
include(EthUtils)

# Create license.h from LICENSE.txt and template
# Converting to char array is required due to MSVC's string size limit.
file(READ ${CMAKE_SOURCE_DIR}/LICENSE LICENSE_TEXT HEX)
string(REGEX MATCHALL ".." LICENSE_TEXT "${LICENSE_TEXT}")
string(REGEX REPLACE ";" ",\n\t0x" LICENSE_TEXT "${LICENSE_TEXT}")
set(LICENSE_TEXT "0x${LICENSE_TEXT}")

configure_file("${CMAKE_SOURCE_DIR}/solidity/cmake/templates/license.h.in" include/license.h)

include(EthOptions)

configure_project(TESTS)

# include_directories(AFTER ${PROJECT_SOURCE_DIR}/solidity)

add_subdirectory(libsolutil)
add_subdirectory(liblangutil)
add_subdirectory(libyul)
add_subdirectory(libevmasm)
add_subdirectory(libsmtutil)
add_subdirectory(libsolidity)
add_subdirectory(libsolc)

if (NOT EMSCRIPTEN)
	add_subdirectory(solppc)
endif()

if (TESTS AND NOT EMSCRIPTEN)
	add_subdirectory(test)
endif()
