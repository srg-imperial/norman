#include "VarDecl.h"

#include "../check/Reference.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <string>

std::optional<transform::VarDeclConfig> transform::VarDeclConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::VarDeclConfig>(v, [](auto& config, auto const& member) {
		if(member.name == "remove local const") {
			if(member.value.IsBool()) {
				config.removeLocalConst = member.value.GetBool();
				return true;
			}
		}
		if(member.name == "graceful") {
			if(member.value.IsBool()) {
				config.graceful = member.value.GetBool();
				return true;
			}
		}
		return false;
	});
}

namespace {
	std::string split(transform::VarDeclConfig const& config, Context& ctx, clang::VarDecl& varDecl) {
		clang::QualType type = varDecl.getType().getDesugaredType(*ctx.astContext);
		if(type.isLocalConstQualified()) {
			if(config.removeLocalConst) {
				type.removeLocalConst();
			} else {
				if(config.graceful) {
					return {};
				} else {
					throw "Split of const variable declaration required";
				}
			}
		}
		clang::VarDecl* vd = clang::VarDecl::Create(*ctx.astContext, varDecl.getDeclContext(), clang::SourceLocation(),
		                                            clang::SourceLocation(), varDecl.getIdentifier(), type, nullptr,
		                                            varDecl.getStorageClass());

		return fmt::format("{};\n{} = ({});", *vd, varDecl.getName(), ctx.source_text(varDecl.getInit()->getSourceRange()));
	}
} // namespace

StmtTransformResult transform::transformVarDecl(VarDeclConfig const& config, Context& ctx, clang::VarDecl& varDecl) {
	if(!config.enabled) {
		return {};
	}

	if(clang::Expr* init = varDecl.getInit()) {
		if(varDecl.isLocalVarDecl() && !varDecl.isStaticLocal()) {
			if(llvm::isa<clang::InitListExpr>(init)) {
				if(config.graceful) {
					return {};
				} else {
					throw "unimplemented";
				}
			} else {
				// SLIGHT SEMANTIC CHANGE if the variable is `const`
				// TODO: only transform if there is a reason to
				return {split(config, ctx, varDecl)};
			}
		}
	}
	return {};
}
