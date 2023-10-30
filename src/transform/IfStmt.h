#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	struct IfStmtConfig : BaseConfig {
		static std::optional<IfStmtConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformIfStmt(IfStmtConfig const& config, clang::ASTContext& astContext, clang::IfStmt& ifStmt);
} // namespace transform