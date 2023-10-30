#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct CallArgConfig : BaseConfig {
		static std::optional<CallArgConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformCallArg(CallArgConfig const& config, clang::ASTContext& astContext, clang::Expr& arg);
} // namespace transform