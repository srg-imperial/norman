#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	struct UnaryExprOrTypeTraitExprConfig : BaseConfig {
		// Currently disabled by default, as we do not emit the correct integer suffixes. (E.g., on typical x86_64
		// configurations, `sizeof(char)` should be `1ul` rather than just `1`.)
		UnaryExprOrTypeTraitExprConfig()
		  : BaseConfig(false) { }

		static std::optional<UnaryExprOrTypeTraitExprConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExprConfig const& config, Context& ctx,
	                                                      clang::UnaryExprOrTypeTraitExpr& expr);
} // namespace transform