#pragma once

#include <cassert>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <string_view>
#include <utility>

#define LOG_HELPER_CONCAT2(a, b) a##b
#define LOG_HELPER_CONCAT(a, b) LOG_HELPER_CONCAT2(a, b)

#define logln(...) (Logger::log_line(__FILE__, "@", __LINE__ __VA_OPT__(, ": ", ) __VA_ARGS__))
#define logScope(name, ...)                                                                                            \
	auto LOG_HELPER_CONCAT(_log_scope_destructor, __COUNTER__) =                                                         \
	  (Logger::enter_scope(__FILE__, __LINE__, name __VA_OPT__(, ) __VA_ARGS__), Logger::ExitScope(name))
#define logFnScope(...) logScope(__PRETTY_FUNCTION__ __VA_OPT__(, " ", ) __VA_ARGS__)

class Logger {
	static std::string& prefix() noexcept {
		static std::string prefix;
		return prefix;
	}

public:
	class ExitScope {
		std::string_view name;

	public:
		explicit ExitScope(std::string_view name) noexcept
		  : name(name) { }

		~ExitScope() {
			prefix().resize(prefix().size() - (sizeof("│") - 1));
			Logger::log_line("╰", "Exited ", name);
		}
	};

	template <typename... ARGS>
	static void enter_scope(char const* file, unsigned line, std::string_view name, ARGS&&... args) {
		Logger::log_line("╭", file, "@", line, ": Entered ", name, std::forward<ARGS>(args)...);
		prefix() += "│";
	}

#ifdef DEBUG
	template <typename... ARGS> static void log_line(ARGS&&... args) {
		auto& out = llvm::outs();
		out << prefix();
		log_line_rec(out, std::forward<ARGS>(args)...);
		out << "\n";
	}
#else
	template <typename... ARGS> static void log_line(ARGS&&...) { }
#endif

private:
	template <typename OS, typename T, typename... ARGS> static void log_line_rec(OS& out, T&& t, ARGS&&... args) {
		out << t;
		log_line_rec(out, std::forward<ARGS>(args)...);
	}

	template <typename OS> static void log_line_rec(OS&) { }
};

class DisplaySourceLoc {
	clang::SourceManager* sourceManager;
	clang::SourceLocation loc;

public:
	DisplaySourceLoc(clang::SourceManager* sourceManager, clang::SourceLocation loc)
	  : sourceManager(sourceManager)
	  , loc(loc) { }

	DisplaySourceLoc(clang::ASTContext* astContext, clang::SourceLocation loc)
	  : sourceManager(&astContext->getSourceManager())
	  , loc(loc) { }

	template <typename T> friend T& operator<<(T& out, DisplaySourceLoc const& self) {
		out << self.sourceManager->getFilename(self.loc) << "@" << self.sourceManager->getSpellingLineNumber(self.loc)
		    << ":" << self.sourceManager->getSpellingColumnNumber(self.loc);
		return out;
	}
};
