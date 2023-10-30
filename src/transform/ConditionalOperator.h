#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct ConditionalOperatorConfig : BaseConfig {
		static std::optional<ConditionalOperatorConfig> parse(rapidjson::Value const&);
	};

	std::optional<TransformationResult> transformConditionalOperator(ConditionalOperatorConfig const& config,
	                                                                 clang::ASTContext& astContext,
	                                                                 clang::ConditionalOperator& cop);
} // namespace transform