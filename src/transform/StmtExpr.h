#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct StmtExprConfig : BaseConfig {
		static std::optional<StmtExprConfig> parse(rapidjson::Value const&);
	};

	ExprTransformResult transformStmtExpr(StmtExprConfig const& config, Context& ctx, clang::StmtExpr& stmtExpr);
} // namespace transform