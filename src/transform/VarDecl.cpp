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
				throw "Split of const variable declaration required";
			}
		}

		clang::VarDecl* vd = clang::VarDecl::Create(*ctx.astContext, varDecl.getDeclContext(), clang::SourceLocation(),
		                                            clang::SourceLocation(), varDecl.getIdentifier(), type, nullptr,
		                                            varDecl.getStorageClass());

		if(auto init = varDecl.getInit(); llvm::isa<clang::InitListExpr>(init)) {
			if(type.getTypePtr()->isArrayType()) {
				char const* volatileString = type.isVolatileQualified() ? "volatile" : "";
				std::string indexName = ctx.uid("_VarDecl_i");
				std::string ptrName = ctx.uid("_VarDecl_p");

				auto typeSizeInBits = ctx.astContext->getTypeSize(type.getTypePtr());
				assert(typeSizeInBits % 8 == 0);

				return fmt::format("{};\n{} {};char {}* {}; {} = (char {}*)&({}){};\nfor({} = 0; {} < {}; ++{}) {{\n((char "
				                   "{}*)&{})[{}] = {}[{}];\n}};",
				                   *vd, static_cast<clang::QualType>(ctx.astContext->getSizeType()).getAsString(), indexName,
				                   volatileString, ptrName, ptrName, volatileString, type.getAsString(),
				                   ctx.source_text(init->getSourceRange()), indexName, indexName, typeSizeInBits / 8, indexName,
				                   volatileString, varDecl.getName(), indexName, ptrName, indexName);
			} else {
				return fmt::format("{};\n{} = ({}){};", *vd, varDecl.getName(), type.getAsString(),
				                   ctx.source_text(init->getSourceRange()));
			}
		} else {
			return fmt::format("{};\n{} = ({});", *vd, varDecl.getName(), ctx.source_text(init->getSourceRange()));
		}
	}
} // namespace

StmtTransformResult transform::transformVarDecl(VarDeclConfig const& config, Context& ctx, clang::VarDecl& varDecl) {
	if(!config.enabled) {
		return {};
	}

	if(varDecl.isLocalVarDecl() && !varDecl.isStaticLocal() && varDecl.getInit()) {
		return {split(config, ctx, varDecl)};
	} else {
		return {};
	}
}
