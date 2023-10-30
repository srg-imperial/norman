#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct ParenExprConfig : BaseConfig {
		static std::optional<ParenExprConfig> parse(rapidjson::Value const&);
	};

	std::optional<TransformationResult> transformParenExpr(ParenExprConfig const& config, clang::ASTContext& astContext,
	                                                       clang::ParenExpr& pexpr);
} // namespace transform