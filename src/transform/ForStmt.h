#pragma once

#include "../BaseConfig.h"
#include "../Context.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct ForStmtConfig : BaseConfig {
		static std::optional<ForStmtConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformForStmt(ForStmtConfig const& config, Context& ctx, clang::ForStmt& forStmt);
} // namespace transform