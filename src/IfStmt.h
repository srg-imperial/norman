#pragma once

#include "NormaliseExprTransform.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>

#include <optional>
#include <string>

std::optional<std::string> transformIfStmt(clang::ASTContext* astContext, clang::IfStmt* ifStmt);
