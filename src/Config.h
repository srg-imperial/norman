#pragma once

#include "transform/CallArg.h"
#include "transform/CommaOperator.h"
#include "transform/ConditionalOperator.h"
#include "transform/DeclStmt.h"
#include "transform/DoStmt.h"
#include "transform/ForStmt.h"
#include "transform/IfStmt.h"
#include "transform/LAndOperator.h"
#include "transform/LOrOperator.h"
#include "transform/ParenExpr.h"
#include "transform/ReturnStmt.h"
#include "transform/StmtExpr.h"
#include "transform/StringLiteral.h"
#include "transform/SwitchStmt.h"
#include "transform/UnaryExprOrTypeTraitExpr.h"
#include "transform/VarDecl.h"
#include "transform/WhileStmt.h"

#include <clang/AST/AST.h>

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class Config {
public:
	struct ScopedConfig {
		bool process = true;

		transform::CallArgConfig configCallArg;
		transform::CommaOperatorConfig configCommaOperator;
		transform::ConditionalOperatorConfig configConditionalOperator;
		transform::DeclStmtConfig configDeclStmt;
		transform::DoStmtConfig configDoStmt;
		transform::ForStmtConfig configForStmt;
		transform::IfStmtConfig configIfStmt;
		transform::LAndOperatorConfig configLAndOperator;
		transform::LOrOperatorConfig configLOrOperator;
		transform::ParenExprConfig configParenExpr;
		transform::ReturnStmtConfig configReturnStmt;
		transform::StmtExprConfig configStmtExpr;
		transform::StringLiteralConfig configStringLiteral;
		transform::SwitchStmtConfig configSwitchStmt;
		transform::UnaryExprOrTypeTraitExprConfig configUnaryExprOrTypeTraitExpr;
		transform::VarDeclConfig configVarDecl;
		transform::WhileStmtConfig configWhileStmt;
	};

	class Filter {
	public:
		virtual bool matches(clang::FunctionDecl const& fdecl) const = 0;
		virtual ~Filter();
	};

private:
	ScopedConfig default_;
	ScopedConfig fileScope_;
	std::vector<std::pair<std::unique_ptr<Filter>, ScopedConfig>> filters;

public:
	static std::optional<Config> from_file(std::string const& filter_path);

	ScopedConfig const& function(clang::FunctionDecl const& fdecl) const {
		for(auto const& [filter, functionConfig] : filters) {
			if(filter->matches(fdecl)) {
				return functionConfig;
			}
		}

		return default_;
	}

	ScopedConfig const& fileScope() const { return fileScope_; }
};
