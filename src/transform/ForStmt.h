#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	struct ForStmtConfig : BaseConfig {
		static std::optional<ForStmtConfig> parse(rapidjson::Value const&);
	};

	std::optional<std::string> transformForStmt(ForStmtConfig const& config, clang::ASTContext& astContext,
	                                            clang::ForStmt& forStmt);
} // namespace transform