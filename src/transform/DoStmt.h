#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct DoStmtConfig : BaseConfig {
		static std::optional<DoStmtConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformDoStmt(DoStmtConfig const& config, clang::ASTContext& astContext, clang::DoStmt& doStmt);
} // namespace transform