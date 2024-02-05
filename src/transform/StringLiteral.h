#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct StringLiteralConfig : BaseConfig {
		static std::optional<StringLiteralConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformStringLiteral(StringLiteralConfig const& config, Context& ctx,
	                                           clang::StringLiteral& strLit);
} // namespace transform