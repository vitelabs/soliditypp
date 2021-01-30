set(sources
	Assembly.cpp
	Assembly.h
	AssemblyItem.cpp
	AssemblyItem.h
	BlockDeduplicator.cpp
	BlockDeduplicator.h
	CommonSubexpressionEliminator.cpp
	CommonSubexpressionEliminator.h
	ConstantOptimiser.cpp
	ConstantOptimiser.h
	ControlFlowGraph.cpp
	ControlFlowGraph.h
	Exceptions.h
	ExpressionClasses.cpp
	ExpressionClasses.h
	GasMeter.cpp
	GasMeter.h
	Instruction.cpp
	Instruction.h
	JumpdestRemover.cpp
	JumpdestRemover.h
	KnownState.cpp
	KnownState.h
	LinkerObject.cpp
	LinkerObject.h
	PathGasMeter.cpp
	PathGasMeter.h
	PeepholeOptimiser.cpp
	PeepholeOptimiser.h
	SemanticInformation.cpp
	SemanticInformation.h
	SimplificationRule.h
	SimplificationRules.cpp
	SimplificationRules.h
)

# Set sources as My.h -> ${PROJECT_SOURCE_DIR}/solidity/lib/My.h
set(DIR ${PROJECT_SOURCE_DIR}/solidity/libevmasm)
list (TRANSFORM sources PREPEND "${DIR}/")

include_directories(${PROJECT_SOURCE_DIR}/solidity)

add_library(evmasm ${sources})
target_link_libraries(evmasm PUBLIC solutil)
