#pragma once

#include "../Norman.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	std::optional<std::string> transformDoStmt(clang::ASTContext* astContext, clang::DoStmt* doStmt);
}