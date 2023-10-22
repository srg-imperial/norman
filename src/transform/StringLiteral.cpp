#include "StringLiteral.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<TransformationResult> transform::transformStringLiteral(clang::ASTContext* astContext,
                                                                      clang::StringLiteral* strLit) {
	if(strLit->getNumConcatenated() != 1) {
		std::string result;
		{
			llvm::raw_string_ostream out{result};
			strLit->printPretty(out, nullptr, astContext->getPrintingPolicy());
		}
		return TransformationResult{std::move(result), {}};
	} else {
		return {};
	}
}
