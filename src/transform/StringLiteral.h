#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct StringLiteralConfig : BaseConfig {
		static std::optional<StringLiteralConfig> parse(rapidjson::Value const&);
	};

	std::optional<TransformationResult> transformStringLiteral(StringLiteralConfig const& config,
	                                                           clang::ASTContext& astContext,
	                                                           clang::StringLiteral& strLit);
} // namespace transform