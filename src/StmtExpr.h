#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

std::optional<TransformationResult> transformStmtExpr(clang::ASTContext* astContext, clang::StmtExpr* stmtExpr);
