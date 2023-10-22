#pragma once

#include "../Norman.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	std::optional<std::string> transformWhileStmt(clang::ASTContext* astContext, clang::WhileStmt* whileStmt);
}