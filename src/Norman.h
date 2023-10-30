#pragma once

#include <string>

struct ExprTransformResult {
	bool do_rewrite;

	std::string expression;
	std::string to_hoist;

	ExprTransformResult()
	  : do_rewrite(false) { }

	ExprTransformResult(std::string expression)
	  : do_rewrite(true)
	  , expression(std::move(expression)) { }

	ExprTransformResult(std::string expression, std::string to_hoist)
	  : do_rewrite(true)
	  , expression(std::move(expression))
	  , to_hoist(std::move(to_hoist)) { }
};

struct StmtTransformResult {
	bool do_rewrite;

	std::string statement;

	StmtTransformResult()
	  : do_rewrite(false) { }

	StmtTransformResult(std::string statement)
	  : do_rewrite(true)
	  , statement(std::move(statement)) { }
};