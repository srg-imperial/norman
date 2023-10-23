#pragma once

#include "../Norman.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>

namespace transform {
	std::optional<TransformationResult> transformStmtExpr(clang::ASTContext* astContext, clang::StmtExpr* stmtExpr);
}