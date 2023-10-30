#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	struct WhileStmtConfig : BaseConfig {
		static std::optional<WhileStmtConfig> parse(rapidjson::Value const&);
	};

	std::optional<std::string> transformWhileStmt(WhileStmtConfig const& config, clang::ASTContext& astContext,
	                                              clang::WhileStmt& whileStmt);
} // namespace transform