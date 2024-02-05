#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct LAndOperatorConfig : BaseConfig {
		static std::optional<LAndOperatorConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformLAndOperator(LAndOperatorConfig const& config, Context& ctx,
	                                          clang::BinaryOperator& binop);
} // namespace transform