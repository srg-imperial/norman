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
	struct ReturnStmtConfig : BaseConfig {
		static std::optional<ReturnStmtConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformReturnStmt(ReturnStmtConfig const& config, Context& ctx, clang::ReturnStmt& returnStmt);
} // namespace transform