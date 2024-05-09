#include "DeclStmt.h"

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

std::optional<transform::DeclStmtConfig> transform::DeclStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::DeclStmtConfig>(v, [](auto& config, auto const& member) { return false; });
}

StmtTransformResult transform::transformDeclStmt(DeclStmtConfig const& config, Context& ctx,
                                                 clang::DeclStmt& declStmt) {
	if(!config.enabled) {
		return {};
	}

	if(declStmt.decls().end() - declStmt.decls().begin() > 1) {
		std::string result;

		for(auto decl : declStmt.decls()) {
			if(auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(decl)) {
				if(recordDecl->getName().empty()) {
					auto id = &ctx.astContext->Idents.get(ctx.uid("_Decl"));
					recordDecl->setDeclName(clang::DeclarationName{id});
				}
				fmt::format_to(std::back_inserter(result), "{};", *recordDecl);
			} else if(auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl)) {
				if(enumDecl->getName().empty()) {
					auto id = &ctx.astContext->Idents.get(ctx.uid("_Decl"));
					enumDecl->setDeclName(clang::DeclarationName{id});
				}
				fmt::format_to(std::back_inserter(result), "{};", *enumDecl);
			} else if(!llvm::isa<clang::EmptyDecl>(decl)) {
				fmt::format_to(std::back_inserter(result), "{};", *decl);
			}
		}
		return result;
	} else {
		return {};
	}
}
