#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

std::optional<std::string> transformReturnStmt(clang::ASTContext* astContext, clang::ReturnStmt* returnStmt);
