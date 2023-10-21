#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

std::optional<TransformationResult> transformCallArg(clang::ASTContext* astContext, clang::Expr* arg);
