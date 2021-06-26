set(ORIGINAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/solidity/libyul)

# This will re-generate the polyfill headers, if any file within libyul/backends/wasm/polyfill/ was modified.
set_directory_properties(PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/solidity/libyul/backends/wasm/polyfill/)

set(POLYFILLS Arithmetic Bitwise Comparison Conversion Interface Keccak Logical Memory)
set(GENERATED_POLYFILL_HEADERS)
foreach(polyfill IN LISTS POLYFILLS)
	set(POLYFILL_FILE ${CMAKE_SOURCE_DIR}/solidity/libyul/backends/wasm/polyfill/${polyfill}.yul)
	file(READ ${POLYFILL_FILE} EWASM_POLYFILL_CONTENT HEX)
	string(REGEX MATCHALL ".." EWASM_POLYFILL_CONTENT "${EWASM_POLYFILL_CONTENT}")
	string(REGEX REPLACE ";" ",\n\t0x" EWASM_POLYFILL_CONTENT "${EWASM_POLYFILL_CONTENT}")
	set(EWASM_POLYFILL_CONTENT "0x${EWASM_POLYFILL_CONTENT}")
	set(EWASM_POLYFILL_NAME ${polyfill})
	configure_file("${CMAKE_SOURCE_DIR}/solidity/cmake/templates/ewasm_polyfill.in" ${CMAKE_BINARY_DIR}/include/ewasmPolyfills/${polyfill}.h @ONLY)
	list(APPEND GENERATED_POLYFILL_HEADERS ${CMAKE_BINARY_DIR}/include/ewasmPolyfills/${polyfill}.h)
endforeach()

set(sources
	${ORIGINAL_SOURCE_DIR}/AsmAnalysis.cpp
	${ORIGINAL_SOURCE_DIR}/AsmAnalysis.h
	${ORIGINAL_SOURCE_DIR}/AsmAnalysisInfo.h
	${ORIGINAL_SOURCE_DIR}/AST.h
	${ORIGINAL_SOURCE_DIR}/ASTForward.h
	${ORIGINAL_SOURCE_DIR}/AsmJsonConverter.h
	${ORIGINAL_SOURCE_DIR}/AsmJsonConverter.cpp
	${ORIGINAL_SOURCE_DIR}/AsmJsonImporter.h
	${ORIGINAL_SOURCE_DIR}/AsmJsonImporter.cpp
	${ORIGINAL_SOURCE_DIR}/AsmParser.cpp
	${ORIGINAL_SOURCE_DIR}/AsmParser.h
	${ORIGINAL_SOURCE_DIR}/AsmPrinter.cpp
	${ORIGINAL_SOURCE_DIR}/AsmPrinter.h
	${ORIGINAL_SOURCE_DIR}/AssemblyStack.h
	${ORIGINAL_SOURCE_DIR}/AssemblyStack.cpp
	${ORIGINAL_SOURCE_DIR}/CompilabilityChecker.cpp
	${ORIGINAL_SOURCE_DIR}/CompilabilityChecker.h
	${ORIGINAL_SOURCE_DIR}/Dialect.cpp
	${ORIGINAL_SOURCE_DIR}/Dialect.h
	${ORIGINAL_SOURCE_DIR}/Exceptions.h
	${ORIGINAL_SOURCE_DIR}/Object.cpp
	${ORIGINAL_SOURCE_DIR}/Object.h
	${ORIGINAL_SOURCE_DIR}/ObjectParser.cpp
	${ORIGINAL_SOURCE_DIR}/ObjectParser.h
	${ORIGINAL_SOURCE_DIR}/Scope.cpp
	${ORIGINAL_SOURCE_DIR}/Scope.h
	${ORIGINAL_SOURCE_DIR}/ScopeFiller.cpp
	${ORIGINAL_SOURCE_DIR}/ScopeFiller.h
	${ORIGINAL_SOURCE_DIR}/Utilities.cpp
	${ORIGINAL_SOURCE_DIR}/Utilities.h
	${ORIGINAL_SOURCE_DIR}/YulString.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/AbstractAssembly.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/AsmCodeGen.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/AsmCodeGen.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/ConstantOptimiser.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/ConstantOptimiser.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMAssembly.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMAssembly.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMCodeTransform.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMCodeTransform.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMDialect.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMDialect.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMObjectCompiler.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMObjectCompiler.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMMetrics.cpp
	${ORIGINAL_SOURCE_DIR}/backends/evm/EVMMetrics.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/NoOutputAssembly.h
	${ORIGINAL_SOURCE_DIR}/backends/evm/NoOutputAssembly.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/EVMToEwasmTranslator.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/EVMToEwasmTranslator.h
	${ORIGINAL_SOURCE_DIR}/backends/wasm/BinaryTransform.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/BinaryTransform.h
	${ORIGINAL_SOURCE_DIR}/backends/wasm/TextTransform.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/TextTransform.h
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WasmCodeTransform.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WasmCodeTransform.h
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WasmDialect.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WasmDialect.h
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WasmObjectCompiler.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WasmObjectCompiler.h
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WordSizeTransform.cpp
	${ORIGINAL_SOURCE_DIR}/backends/wasm/WordSizeTransform.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ASTCopier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ASTCopier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ASTWalker.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ASTWalker.h
	${ORIGINAL_SOURCE_DIR}/optimiser/BlockFlattener.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/BlockFlattener.h
	${ORIGINAL_SOURCE_DIR}/optimiser/BlockHasher.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/BlockHasher.h
	${ORIGINAL_SOURCE_DIR}/optimiser/CallGraphGenerator.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/CallGraphGenerator.h
	${ORIGINAL_SOURCE_DIR}/optimiser/CircularReferencesPruner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/CircularReferencesPruner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/CommonSubexpressionEliminator.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/CommonSubexpressionEliminator.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ConditionalSimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ConditionalSimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ConditionalUnsimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ConditionalUnsimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ControlFlowSimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ControlFlowSimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/DataFlowAnalyzer.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/DataFlowAnalyzer.h
	${ORIGINAL_SOURCE_DIR}/optimiser/DeadCodeEliminator.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/DeadCodeEliminator.h
	${ORIGINAL_SOURCE_DIR}/optimiser/Disambiguator.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/Disambiguator.h
	${ORIGINAL_SOURCE_DIR}/optimiser/EquivalentFunctionDetector.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/EquivalentFunctionDetector.h
	${ORIGINAL_SOURCE_DIR}/optimiser/EquivalentFunctionCombiner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/EquivalentFunctionCombiner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionInliner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionInliner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionJoiner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionJoiner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionSimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionSimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionSplitter.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ExpressionSplitter.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ForLoopConditionIntoBody.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ForLoopConditionIntoBody.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ForLoopConditionOutOfBody.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ForLoopConditionOutOfBody.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ForLoopInitRewriter.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ForLoopInitRewriter.h
	${ORIGINAL_SOURCE_DIR}/optimiser/FullInliner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/FullInliner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/FunctionCallFinder.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/FunctionCallFinder.h
	${ORIGINAL_SOURCE_DIR}/optimiser/FunctionGrouper.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/FunctionGrouper.h
	${ORIGINAL_SOURCE_DIR}/optimiser/FunctionHoister.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/FunctionHoister.h
	${ORIGINAL_SOURCE_DIR}/optimiser/InlinableExpressionFunctionFinder.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/InlinableExpressionFunctionFinder.h
	${ORIGINAL_SOURCE_DIR}/optimiser/KnowledgeBase.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/KnowledgeBase.h
	${ORIGINAL_SOURCE_DIR}/optimiser/LoadResolver.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/LoadResolver.h
	${ORIGINAL_SOURCE_DIR}/optimiser/LoopInvariantCodeMotion.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/LoopInvariantCodeMotion.h
	${ORIGINAL_SOURCE_DIR}/optimiser/MainFunction.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/MainFunction.h
	${ORIGINAL_SOURCE_DIR}/optimiser/Metrics.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/Metrics.h
	${ORIGINAL_SOURCE_DIR}/optimiser/NameCollector.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/NameCollector.h
	${ORIGINAL_SOURCE_DIR}/optimiser/NameDispenser.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/NameDispenser.h
	${ORIGINAL_SOURCE_DIR}/optimiser/NameDisplacer.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/NameDisplacer.h
	${ORIGINAL_SOURCE_DIR}/optimiser/NameSimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/NameSimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/OptimiserStep.h
	${ORIGINAL_SOURCE_DIR}/optimiser/OptimizerUtilities.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/OptimizerUtilities.h
	${ORIGINAL_SOURCE_DIR}/optimiser/ReasoningBasedSimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/ReasoningBasedSimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/RedundantAssignEliminator.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/RedundantAssignEliminator.h
	${ORIGINAL_SOURCE_DIR}/optimiser/Rematerialiser.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/Rematerialiser.h
	${ORIGINAL_SOURCE_DIR}/optimiser/SSAReverser.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/SSAReverser.h
	${ORIGINAL_SOURCE_DIR}/optimiser/SSATransform.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/SSATransform.h
	${ORIGINAL_SOURCE_DIR}/optimiser/SSAValueTracker.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/SSAValueTracker.h
	${ORIGINAL_SOURCE_DIR}/optimiser/Semantics.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/Semantics.h
	${ORIGINAL_SOURCE_DIR}/optimiser/SimplificationRules.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/SimplificationRules.h
	${ORIGINAL_SOURCE_DIR}/optimiser/StackCompressor.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/StackCompressor.h
	${ORIGINAL_SOURCE_DIR}/optimiser/StackLimitEvader.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/StackLimitEvader.h
	${ORIGINAL_SOURCE_DIR}/optimiser/StackToMemoryMover.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/StackToMemoryMover.h
	${ORIGINAL_SOURCE_DIR}/optimiser/StructuralSimplifier.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/StructuralSimplifier.h
	${ORIGINAL_SOURCE_DIR}/optimiser/Substitution.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/Substitution.h
	${ORIGINAL_SOURCE_DIR}/optimiser/Suite.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/Suite.h
	${ORIGINAL_SOURCE_DIR}/optimiser/SyntacticalEquality.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/SyntacticalEquality.h
	${ORIGINAL_SOURCE_DIR}/optimiser/TypeInfo.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/TypeInfo.h
	${ORIGINAL_SOURCE_DIR}/optimiser/UnusedFunctionParameterPruner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/UnusedFunctionParameterPruner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/UnusedFunctionsCommon.h
	${ORIGINAL_SOURCE_DIR}/optimiser/UnusedFunctionsCommon.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/UnusedPruner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/UnusedPruner.h
	${ORIGINAL_SOURCE_DIR}/optimiser/VarDeclInitializer.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/VarDeclInitializer.h
	${ORIGINAL_SOURCE_DIR}/optimiser/VarNameCleaner.cpp
	${ORIGINAL_SOURCE_DIR}/optimiser/VarNameCleaner.h
)

include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_library(yul ${GENERATED_POLYFILL_HEADERS} ${sources})
target_link_libraries(yul PUBLIC evmasm solutil langutil smtutil)
