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
	struct DeclStmtConfig : BaseConfig {
		bool removeLocalConst = false; ///< potentially changes semantics, when enabled

		bool graceful = false; ///< do not crash if DeclStmt splitting fails

		static std::optional<DeclStmtConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformDeclStmt(DeclStmtConfig const& config, Context& ctx, clang::DeclStmt& declStmt);
} // namespace transform