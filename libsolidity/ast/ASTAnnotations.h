// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2015
 * Object containing the type and other annotations for the AST nodes.
 */

#pragma once

#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/ASTEnums.h>
#include <libsolidity/ast/ExperimentalFeatures.h>

#include <libsolutil/SetOnce.h>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace solidity::yul
{
struct AsmAnalysisInfo;
struct Identifier;
struct Dialect;
}

namespace solidity::frontend
{

class Type;
using TypePointer = Type const*;
using namespace util;

struct ASTAnnotation
{
	ASTAnnotation() = default;

	ASTAnnotation(ASTAnnotation const&) = delete;
	ASTAnnotation(ASTAnnotation&&) = delete;

	ASTAnnotation& operator=(ASTAnnotation const&) = delete;
	ASTAnnotation& operator=(ASTAnnotation&&) = delete;

	virtual ~ASTAnnotation() = default;
};

struct DocTag
{
	std::string content;	///< The text content of the tag.
	std::string paramName;	///< Only used for @param, stores the parameter name.
};

struct StructurallyDocumentedAnnotation
{
	StructurallyDocumentedAnnotation() = default;

	StructurallyDocumentedAnnotation(StructurallyDocumentedAnnotation const&) = delete;
	StructurallyDocumentedAnnotation(StructurallyDocumentedAnnotation&&) = delete;

	StructurallyDocumentedAnnotation& operator=(StructurallyDocumentedAnnotation const&) = delete;
	StructurallyDocumentedAnnotation& operator=(StructurallyDocumentedAnnotation&&) = delete;

	virtual ~StructurallyDocumentedAnnotation() = default;

	/// Mapping docstring tag name -> content.
	std::multimap<std::string, DocTag> docTags;
	/// contract that @inheritdoc references if it exists
	ContractDefinition const* inheritdocReference = nullptr;
};

struct SourceUnitAnnotation: ASTAnnotation
{
	/// The "absolute" (in the compiler sense) path of this source unit.
	SetOnce<std::string> path;
	/// The exported symbols (all global symbols).
	SetOnce<std::map<ASTString, std::vector<Declaration const*>>> exportedSymbols;
	/// Experimental features.
	std::set<ExperimentalFeature> experimentalFeatures;
	/// Using the new ABI coder. Set to `false` if using ABI coder v1.
	SetOnce<bool> useABICoderV2;
	/// Solidity++: The programing language of this source unit
	SetOnce<SourceLanguage> sourceLanguage;
};

struct ScopableAnnotation
{
	ScopableAnnotation() = default;

	ScopableAnnotation(ScopableAnnotation const&) = delete;
	ScopableAnnotation(ScopableAnnotation&&) = delete;

	ScopableAnnotation& operator=(ScopableAnnotation const&) = delete;
	ScopableAnnotation& operator=(ScopableAnnotation&&) = delete;

	virtual ~ScopableAnnotation() = default;

	/// The scope this declaration resides in. Can be nullptr if it is the global scope.
	/// Filled by the Scoper.
	ASTNode const* scope = nullptr;
	/// Pointer to the contract this declaration resides in. Can be nullptr if the current scope
	/// is not part of a contract. Filled by the Scoper.
	ContractDefinition const* contract = nullptr;
};

struct DeclarationAnnotation: ASTAnnotation, ScopableAnnotation
{
};

struct ImportAnnotation: DeclarationAnnotation
{
	/// The absolute path of the source unit to import.
	SetOnce<std::string> absolutePath;
	/// The actual source unit.
	SourceUnit const* sourceUnit = nullptr;
};

struct TypeDeclarationAnnotation: DeclarationAnnotation
{
	/// The name of this type, prefixed by proper namespaces if globally accessible.
	SetOnce<std::string> canonicalName;
};

struct StructDeclarationAnnotation: TypeDeclarationAnnotation
{
	/// Whether the struct is recursive, i.e. if the struct (recursively) contains a member that involves a struct of the same
	/// type, either in a dynamic array, as member of another struct or inside a mapping.
	/// Only cases in which the recursive occurrence is within a dynamic array or a mapping are valid, while direct
	/// recursion immediately raises an error.
	/// Will be filled in by the DeclarationTypeChecker.
	std::optional<bool> recursive;
	/// Whether the struct contains a mapping type, either directly or, indirectly inside another
	/// struct or an array.
	std::optional<bool> containsNestedMapping;
};

struct ContractDefinitionAnnotation: TypeDeclarationAnnotation, StructurallyDocumentedAnnotation
{
	/// List of functions and modifiers without a body. Can also contain functions from base classes.
	std::optional<std::vector<Declaration const*>> unimplementedDeclarations;
	/// List of all (direct and indirect) base contracts in order from derived to
	/// base, including the contract itself.
	std::vector<ContractDefinition const*> linearizedBaseContracts;
	/// List of contracts this contract creates, i.e. which need to be compiled first.
	/// Also includes all contracts from @a linearizedBaseContracts.
	std::set<ContractDefinition const*> contractDependencies;
	/// Mapping containing the nodes that define the arguments for base constructors.
	/// These can either be inheritance specifiers or modifier invocations.
	std::map<FunctionDefinition const*, ASTNode const*> baseConstructorArguments;
};

struct CallableDeclarationAnnotation: DeclarationAnnotation
{
	/// The set of functions/modifiers/events this callable overrides.
	std::set<CallableDeclaration const*> baseFunctions;
};

struct FunctionDefinitionAnnotation: CallableDeclarationAnnotation, StructurallyDocumentedAnnotation
{
};

struct EventDefinitionAnnotation: CallableDeclarationAnnotation, StructurallyDocumentedAnnotation
{
};

// Solidity++: 
struct MessageDefinitionAnnotation: CallableDeclarationAnnotation, StructurallyDocumentedAnnotation
{
};

struct ModifierDefinitionAnnotation: CallableDeclarationAnnotation, StructurallyDocumentedAnnotation
{
};

struct VariableDeclarationAnnotation: DeclarationAnnotation, StructurallyDocumentedAnnotation
{
	/// Type of variable (type of identifier referencing this variable).
	TypePointer type = nullptr;
	/// The set of functions this (public state) variable overrides.
	std::set<CallableDeclaration const*> baseFunctions;
};

struct StatementAnnotation: ASTAnnotation
{
};

struct InlineAssemblyAnnotation: StatementAnnotation
{
	struct ExternalIdentifierInfo
	{
		Declaration const* declaration = nullptr;
		/// Suffix used, one of "slot", "offset", "length" or empty.
		std::string suffix;
		size_t valueSize = size_t(-1);
	};

	/// Mapping containing resolved references to external identifiers and their value size
	std::map<yul::Identifier const*, ExternalIdentifierInfo> externalReferences;
	/// Information generated during analysis phase.
	std::shared_ptr<yul::AsmAnalysisInfo> analysisInfo;
};

struct BlockAnnotation: StatementAnnotation, ScopableAnnotation
{
};

struct TryCatchClauseAnnotation: ASTAnnotation, ScopableAnnotation
{
};

struct ForStatementAnnotation: StatementAnnotation, ScopableAnnotation
{
};

struct ReturnAnnotation: StatementAnnotation
{
	/// Reference to the return parameters of the function.
	ParameterList const* functionReturnParameters = nullptr;
};

struct TypeNameAnnotation: ASTAnnotation
{
	/// Type declared by this type name, i.e. type of a variable where this type name is used.
	/// Set during reference resolution stage.
	TypePointer type = nullptr;
};

struct IdentifierPathAnnotation: ASTAnnotation
{
	/// Referenced declaration, set during reference resolution stage.
	Declaration const* referencedDeclaration = nullptr;
	/// What kind of lookup needs to be done (static, virtual, super) find the declaration.
	SetOnce<VirtualLookup> requiredLookup;
};

struct ExpressionAnnotation: ASTAnnotation
{
	/// Inferred type of the expression.
	TypePointer type = nullptr;
	/// Whether the expression is a constant variable
	SetOnce<bool> isConstant;
	/// Whether the expression is pure, i.e. compile-time constant.
	SetOnce<bool> isPure;
	/// Whether it is an LValue (i.e. something that can be assigned to).
	SetOnce<bool> isLValue;
	/// Whether the expression is used in a context where the LValue is actually required.
	bool willBeWrittenTo = false;
	/// Whether the expression is an lvalue that is only assigned.
	/// Would be false for --, ++, delete, +=, -=, ....
	/// Only relevant if isLvalue == true
	bool lValueOfOrdinaryAssignment = false;

	/// Types and - if given - names of arguments if the expr. is a function
	/// that is called, used for overload resolution
	std::optional<FuncCallArguments> arguments;

	/// True if the expression consists solely of the name of the function and the function is called immediately
	/// instead of being stored or processed. The name may be qualified with the name of a contract, library
	/// module, etc., that clarifies the scope. For example: `m.L.f()`, where `m` is a module, `L` is a library
	/// and `f` is a function is a direct call. This means that the function to be called is known at compilation
	/// time and it's not necessary to rely on any runtime dispatch mechanism to resolve it.
	/// Note that even the simplest expressions, like `(f)()`, result in an indirect call even if they consist of
	/// values known at compilation time.
	bool calledDirectly = false;
};

struct IdentifierAnnotation: ExpressionAnnotation
{
	/// Referenced declaration, set at latest during overload resolution stage.
	Declaration const* referencedDeclaration = nullptr;
	/// What kind of lookup needs to be done (static, virtual, super) find the declaration.
	SetOnce<VirtualLookup> requiredLookup;
	/// List of possible declarations it could refer to (can contain duplicates).
	std::vector<Declaration const*> candidateDeclarations;
	/// List of possible declarations it could refer to.
	std::vector<Declaration const*> overloadedDeclarations;
};

struct MemberAccessAnnotation: ExpressionAnnotation
{
	/// Referenced declaration, set at latest during overload resolution stage.
	Declaration const* referencedDeclaration = nullptr;
	/// What kind of lookup needs to be done (static, virtual, super) find the declaration.
	SetOnce<VirtualLookup> requiredLookup;
};

struct BinaryOperationAnnotation: ExpressionAnnotation
{
	/// The common type that is used for the operation, not necessarily the result type (which
	/// e.g. for comparisons is bool).
	TypePointer commonType = nullptr;
};

enum class FunctionCallKind
{
	FunctionCall,
	TypeConversion,
	StructConstructorCall
};

struct FunctionCallAnnotation: ExpressionAnnotation
{
	util::SetOnce<FunctionCallKind> kind;
	/// If true, this is an async call.
	bool async = true;
	/// If true, this is the external call of a try statement.
	bool tryCall = false;
};

}
