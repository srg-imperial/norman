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
	struct VarDeclConfig : BaseConfig {
		bool removeLocalConst = false; ///< potentially changes semantics, when enabled

		bool graceful = false; ///< do not crash if vardecl splitting fails

		static std::optional<VarDeclConfig> parse(rapidjson::Value const&);
	};

	StmtTransformResult transformVarDecl(VarDeclConfig const& config, Context& ctx, clang::VarDecl& varDecl);
} // namespace transform