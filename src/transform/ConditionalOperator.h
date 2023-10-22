#pragma once

#include "../Norman.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	std::optional<TransformationResult> transformConditionalOperator(clang::ASTContext* astContext,
	                                                                 clang::ConditionalOperator* cop);
}