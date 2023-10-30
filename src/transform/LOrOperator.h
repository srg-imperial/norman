#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct LOrOperatorConfig : BaseConfig {
		static std::optional<LOrOperatorConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformLOrOperator(LOrOperatorConfig const& config, clang::ASTContext& astContext,
	                                         clang::BinaryOperator& binop);
} // namespace transform