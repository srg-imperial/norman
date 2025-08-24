#include "StringLiteral.h"

#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::StringLiteralConfig> transform::StringLiteralConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::StringLiteralConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformStringLiteral(StringLiteralConfig const& config, Context& ctx,
                                                      clang::StringLiteral& strLit) {
	if(!config.enabled) {
		return {};
	}

	if(strLit.getNumConcatenated() != 1) {
		std::string result;
		{
			llvm::raw_string_ostream out{result};
			strLit.printPretty(out, nullptr, ctx.astContext->getPrintingPolicy());
		}
		return {std::move(result), {}};
	} else {
		return {};
	}
}
