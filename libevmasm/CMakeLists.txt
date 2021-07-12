set(ORIGINAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/solidity/libevmasm)

set(sources
	Assembly.cpp
	Assembly.h
	${ORIGINAL_SOURCE_DIR}/AssemblyItem.cpp
	${ORIGINAL_SOURCE_DIR}/AssemblyItem.h
	${ORIGINAL_SOURCE_DIR}/BlockDeduplicator.cpp
	${ORIGINAL_SOURCE_DIR}/BlockDeduplicator.h
	${ORIGINAL_SOURCE_DIR}/CommonSubexpressionEliminator.cpp
	${ORIGINAL_SOURCE_DIR}/CommonSubexpressionEliminator.h
	${ORIGINAL_SOURCE_DIR}/ConstantOptimiser.cpp
	${ORIGINAL_SOURCE_DIR}/ConstantOptimiser.h
	${ORIGINAL_SOURCE_DIR}/ControlFlowGraph.cpp
	${ORIGINAL_SOURCE_DIR}/ControlFlowGraph.h
	${ORIGINAL_SOURCE_DIR}/Exceptions.h
	${ORIGINAL_SOURCE_DIR}/ExpressionClasses.cpp
	${ORIGINAL_SOURCE_DIR}/ExpressionClasses.h
	${ORIGINAL_SOURCE_DIR}/GasMeter.cpp
	${ORIGINAL_SOURCE_DIR}/GasMeter.h
	Instruction.cpp
	Instruction.h
	${ORIGINAL_SOURCE_DIR}/JumpdestRemover.cpp
	${ORIGINAL_SOURCE_DIR}/JumpdestRemover.h
	${ORIGINAL_SOURCE_DIR}/KnownState.cpp
	${ORIGINAL_SOURCE_DIR}/KnownState.h
	${ORIGINAL_SOURCE_DIR}/LinkerObject.cpp
	${ORIGINAL_SOURCE_DIR}/LinkerObject.h
	${ORIGINAL_SOURCE_DIR}/PathGasMeter.cpp
	${ORIGINAL_SOURCE_DIR}/PathGasMeter.h
	${ORIGINAL_SOURCE_DIR}/PeepholeOptimiser.cpp
	${ORIGINAL_SOURCE_DIR}/PeepholeOptimiser.h
	${ORIGINAL_SOURCE_DIR}/SemanticInformation.cpp
	${ORIGINAL_SOURCE_DIR}/SemanticInformation.h
	${ORIGINAL_SOURCE_DIR}/SimplificationRule.h
	${ORIGINAL_SOURCE_DIR}/SimplificationRules.cpp
	${ORIGINAL_SOURCE_DIR}/SimplificationRules.h
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_library(evmasm ${sources})
target_link_libraries(evmasm PUBLIC solutil)
