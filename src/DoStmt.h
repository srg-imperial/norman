#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

std::optional<std::string> transformDoStmt(clang::ASTContext* astContext, clang::DoStmt* doStmt);
