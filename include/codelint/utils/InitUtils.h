#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <optional>

namespace clang::tidy::codelint::utils {

/// Check if a location is in a system header
bool isInSystemHeader(SourceLocation Loc, ASTContext* Ctx);

/// Check if the variable type contains auto (including auto*, const auto*, auto&, etc.)
bool shouldSkipAuto(const VarDecl* VarDeclPtr);

/// Check if the variable was declared with auto type (using TypeSourceInfo)
bool isAutoType(const VarDecl* VarDeclPtr);

/// Check if the variable is inside a union
bool shouldSkipUnion(const VarDecl* VarDeclPtr);

/// Check if the variable is an extern declaration
bool shouldSkipExtern(const VarDecl* VarDeclPtr);

/// Check if the variable is an enum class that doesn't have a zero value
bool shouldSkipEnumClass(const VarDecl* VarDeclPtr);

/// Check if an enum type has a zero value enumerator
bool isEnumZeroValidType(const Type* TypePtr);

/// Check if a variable has an explicit initializer (CInit, ListInit, or non-empty CXXConstructExpr)
bool hasExplicitInitializer(const VarDecl* VarDeclPtr);

/// Check if a field has an in-class initializer
bool hasExplicitInitializer(const FieldDecl* FieldDeclPtr);

/// Check if a variable declaration is inside a macro expansion
bool isInsideMacro(const VarDecl* VarDeclPtr, ASTContext* Ctx);

/// Check if a type has a non-trivial default constructor
/// For array types, checks the element type recursively
[[nodiscard]] bool hasNonTrivialDefaultConstructor(QualType QualTypeRef);

/// Check if a CXXRecordDecl has a constructor taking std::initializer_list
/// Returns the element type of the initializer_list if found, or std::nullopt otherwise
[[nodiscard]] std::optional<QualType> hasInitializerListConstructor(const CXXRecordDecl* Record);

} // namespace clang::tidy::codelint::utils
