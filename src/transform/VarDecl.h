#pragma once

#include "../BaseConfig.h"
#include "../Norman.h"

#include <rapidjson/document.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	struct VarDeclConfig : BaseConfig {
		bool removeLocalConst = false; ///< potentially changes semantics, when enabled

		static std::optional<VarDeclConfig> parse(rapidjson::Value const&);
	};

	std::optional<std::string> transformVarDecl(VarDeclConfig const& config, clang::ASTContext& astContext,
	                                            clang::VarDecl& varDecl);
} // namespace transform