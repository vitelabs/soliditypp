# EVMC: Ethereum Client-VM Connector API.
# Copyright 2016-2019 The EVMC Authors.
# Licensed under the Apache License, Version 2.0.


add_library(evmc_loader STATIC ${PROJECT_SOURCE_DIR}/solidity/test/evmc/loader.h ${PROJECT_SOURCE_DIR}/solidity/test/evmc/loader.c)
set_target_properties(evmc_loader PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(evmc_loader INTERFACE ${CMAKE_DL_LIBS})
target_include_directories(evmc_loader PUBLIC ${PROJECT_SOURCE_DIR}/solidity/test/)
set_target_properties(evmc_loader PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
)

add_library(evmc INTERFACE)
target_sources(evmc INTERFACE
    ${PROJECT_SOURCE_DIR}/solidity/test/evmc/evmc.h
    ${PROJECT_SOURCE_DIR}/solidity/test/evmc/evmc.hpp
    ${PROJECT_SOURCE_DIR}/solidity/test/evmc/helpers.h
    ${PROJECT_SOURCE_DIR}/solidity/test/evmc/utils.h
)
target_include_directories(evmc INTERFACE ${PROJECT_SOURCE_DIR}/solidity/test/)
target_link_libraries(evmc INTERFACE evmc_loader)
