#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct ParenExprConfig : BaseConfig {
		static std::optional<ParenExprConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformParenExpr(ParenExprConfig const& config, Context& ctx, clang::ParenExpr& pexpr);
} // namespace transform