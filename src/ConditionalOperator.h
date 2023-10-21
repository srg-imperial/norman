#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

std::optional<TransformationResult> transformConditionalOperator(clang::ASTContext* astContext,
                                                                 clang::ConditionalOperator* cop);
