#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

std::optional<TransformationResult> transformParenExpr(clang::ASTContext* astContext, clang::ParenExpr* pexpr);
