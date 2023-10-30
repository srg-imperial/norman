#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	struct SwitchStmtConfig : BaseConfig {
		static std::optional<SwitchStmtConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformSwitchStmt(SwitchStmtConfig const& config, clang::ASTContext& astContext,
	                                        clang::SwitchStmt& switchStmt);
} // namespace transform