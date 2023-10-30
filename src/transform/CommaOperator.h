#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct CommaOperatorConfig : BaseConfig {
		static std::optional<CommaOperatorConfig> parse(rapidjson::Value const&);
	};

	std::optional<TransformationResult> transformCommaOperator(CommaOperatorConfig const& config,
	                                                           clang::ASTContext& astContext,
	                                                           clang::BinaryOperator& binop);
} // namespace transform