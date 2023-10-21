#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

std::optional<std::string> transformSwitchStmt(clang::ASTContext* astContext, clang::SwitchStmt* switchStmt);
