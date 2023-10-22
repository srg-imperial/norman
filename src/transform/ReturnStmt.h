#pragma once

#include "../Norman.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

namespace transform {
	std::optional<std::string> transformReturnStmt(clang::ASTContext* astContext, clang::ReturnStmt* returnStmt);
}