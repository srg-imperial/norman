#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct CommaOperatorConfig : BaseConfig {
		static std::optional<CommaOperatorConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformCommaOperator(CommaOperatorConfig const& config, Context& ctx,
	                                           clang::BinaryOperator& binop);
} // namespace transform