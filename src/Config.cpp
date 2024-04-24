#include "Config.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

namespace {
	class MultiFilter final : public Config::Filter {
		std::vector<std::unique_ptr<Config::Filter>> filters;

	public:
		bool matches(clang::FunctionDecl const& fdecl) const override {
			for(auto const& filter : filters) {
				if(!filter->matches(fdecl)) {
					return false;
				}
			}
			return true;
		}

		void add_filter(std::unique_ptr<Config::Filter> filter) { filters.emplace_back(std::move(filter)); }
		void take_filters(MultiFilter&& from) {
			for(auto& filter : from.filters) {
				filters.emplace_back(std::move(filter));
			}
		}

		static std::unique_ptr<Config::Filter> merge(std::unique_ptr<Config::Filter> lhs,
		                                             std::unique_ptr<Config::Filter> rhs) {
			if(!lhs) {
				return rhs;
			}
			if(!rhs) {
				return lhs;
			}

			if(auto* multi = dynamic_cast<MultiFilter*>(lhs.get())) {
				if(auto* other = dynamic_cast<MultiFilter*>(lhs.get())) {
					multi->take_filters(std::move(*other));
				} else {
					multi->add_filter(std::move(rhs));
				}
				return lhs;
			}
			if(auto* multi = dynamic_cast<MultiFilter*>(rhs.get())) {
				multi->add_filter(std::move(lhs));
				return rhs;
			}

			auto result = std::make_unique<MultiFilter>();
			result->add_filter(std::move(lhs));
			result->add_filter(std::move(rhs));
			return result;
		}
	};

	class NameFilter final : public Config::Filter {
		std::string name;

	public:
		NameFilter(std::string name)
		  : name(std::move(name)) { }

		bool matches(clang::FunctionDecl const& fdecl) const override { return fdecl.getName() == name; }
	};

	class InlineFilter final : public Config::Filter {
		bool isInline;

	public:
		InlineFilter() = delete;
		InlineFilter(bool isInline)
		  : isInline(isInline) { }

		bool matches(clang::FunctionDecl const& fdecl) const override { return fdecl.isInlineSpecified() == isInline; }
	};

	class ExternFilter final : public Config::Filter {
	public:
		bool matches(clang::FunctionDecl const& fdecl) const override { return fdecl.isExternC(); }
	};

	bool parse_update_config(Config::ScopedConfig& config, rapidjson::Document::Member const& member) {
#define CONFIG_CASE(type)                                                                                              \
	else if(member.name == #type) {                                                                                      \
		if(auto c = transform::type##Config::parse(member.value)) {                                                        \
			config.config##type = ::std::move(*c);                                                                           \
			return true;                                                                                                     \
		}                                                                                                                  \
	}

		if(member.name == "process") {
			if(member.value.IsBool()) {
				config.process = member.value.GetBool();
				return true;
			}
		}
		CONFIG_CASE(CallArg)
		CONFIG_CASE(CommaOperator)
		CONFIG_CASE(ConditionalOperator)
		CONFIG_CASE(DeclStmt)
		CONFIG_CASE(DoStmt)
		CONFIG_CASE(ForStmt)
		CONFIG_CASE(IfStmt)
		CONFIG_CASE(LAndOperator)
		CONFIG_CASE(LOrOperator)
		CONFIG_CASE(ParenExpr)
		CONFIG_CASE(ReturnStmt)
		CONFIG_CASE(StmtExpr)
		CONFIG_CASE(StringLiteral)
		CONFIG_CASE(SwitchStmt)
		CONFIG_CASE(UnaryExprOrTypeTraitExpr)
		CONFIG_CASE(VarDecl)
		CONFIG_CASE(WhileStmt)

		return false;

#undef CONFIG_CASE
	}

	std::optional<Config::ScopedConfig> parse_default(rapidjson::Value const& v) {
		if(!v.IsObject()) {
			return {};
		}

		Config::ScopedConfig config;
		for(auto const& member : v.GetObject()) {
			if(parse_update_config(config, member)) {
				// done
			} else {
				return {};
			}
		}
		return {std::move(config)};
	}

	std::optional<Config::ScopedConfig> parse_file_scope(Config::ScopedConfig const& defaultConfig,
	                                                     rapidjson::Value const& v) {
		if(!v.IsObject()) {
			return {};
		}

		Config::ScopedConfig config = defaultConfig;
		for(auto const& member : v.GetObject()) {
			if(parse_update_config(config, member)) {
				// done
			} else {
				return {};
			}
		}
		return {std::move(config)};
	}

	std::optional<std::pair<std::unique_ptr<Config::Filter>, Config::ScopedConfig>>
	parse_override(Config::ScopedConfig const& defaultConfig, rapidjson::Value const& v) {
		if(!v.IsObject()) {
			return {};
		}

		std::pair<std::unique_ptr<Config::Filter>, Config::ScopedConfig> result{nullptr, defaultConfig};
		for(auto const& member : v.GetObject()) {
			if(member.name == "name") {
				result.first = MultiFilter::merge(
				  std::move(result.first),
				  std::make_unique<NameFilter>(std::string{member.value.GetString(), member.value.GetStringLength()}));
			} else if(member.name == "linkage") {
				if(member.value == "extern") {
					result.first = MultiFilter::merge(std::move(result.first), std::make_unique<ExternFilter>());
				} else {
					return {};
				}
			} else if(member.name == "inline") {
				if(member.value.IsBool()) {
					result.first =
					  MultiFilter::merge(std::move(result.first), std::make_unique<InlineFilter>(member.value.GetBool()));
				} else {
					return {};
				}
			} else if(parse_update_config(result.second, member)) {
				// done
			} else {
				return {};
			}
		}

		return {std::move(result)};
	}
} // namespace

Config::Filter::~Filter() = default;

std::optional<Config> Config::from_file(std::string const& filter_path) {
	if(std::ifstream ifs{filter_path}) {
		Config result;

		rapidjson::IStreamWrapper isw{ifs};
		rapidjson::Document doc;
		doc.ParseStream(isw);

		if(!doc.IsObject()) {
			return {};
		}

		auto o = doc.GetObject();

		if(auto defaultConfig = o.FindMember("default"); defaultConfig != o.end()) {
			if(auto t = parse_default(defaultConfig->value)) {
				result.default_ = std::move(*t);
			} else {
				return {};
			}
		}

		for(auto const& member : o) {
			if(member.name == "default") {
				// we have already processed this member
			} else if(member.name == "functions") {
				if(!member.value.IsArray()) {
					return {};
				}

				for(auto const& override_spec : member.value.GetArray()) {
					if(auto f = parse_override(result.default_, override_spec)) {
						result.filters.push_back(std::move(*f));
					} else {
						return {};
					}
				}
			} else if(member.name == "file scope") {
				if(auto c = parse_file_scope(result.default_, member.value)) {
					result.fileScope_ = std::move(*c);
				} else {
					return {};
				}
			} else {
				return {};
			}
		}

		return {std::move(result)};
	} else {
		return {};
	}
}