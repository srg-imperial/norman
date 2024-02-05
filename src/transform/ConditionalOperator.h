#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct ConditionalOperatorConfig : BaseConfig {
		static std::optional<ConditionalOperatorConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformConditionalOperator(ConditionalOperatorConfig const& config, Context& ctx,
	                                                 clang::ConditionalOperator& cop);
} // namespace transform