#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <clang/AST/AST.h>

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class FunctionFilter {
	enum class Mode {
		Process,
		Skip,
	};

	static std::optional<Mode> parse_mode(std::string_view mode) {
		if(mode == "process") {
			return {Mode::Process};
		} else if(mode == "skip") {
			return {Mode::Skip};
		} else {
			return {};
		}
	}

	class Filter {
	public:
		virtual bool matches(clang::FunctionDecl* fdecl) const = 0;
		virtual ~Filter() = default;

		static inline std::unique_ptr<Filter> combine(std::unique_ptr<Filter> lhs, std::unique_ptr<Filter> rhs);
	};

	class MultiFilter final : public Filter {
		std::vector<std::unique_ptr<Filter>> filters;

	public:
		bool matches(clang::FunctionDecl* fdecl) const override {
			for(auto const& filter : filters) {
				if(!filter->matches(fdecl)) {
					return false;
				}
			}
			return true;
		}

		void add_filter(std::unique_ptr<Filter> filter) { filters.emplace_back(std::move(filter)); }
	};

	class NameFilter final : public Filter {
		std::string name;

	public:
		NameFilter(std::string name)
		  : name(std::move(name)) { }

		bool matches(clang::FunctionDecl* fdecl) const override { return fdecl->getName() == name; }
	};

	class InlineFilter final : public Filter {
	public:
		bool matches(clang::FunctionDecl* fdecl) const override { return fdecl->isInlineSpecified(); }
	};

	class ExternFilter final : public Filter {
	public:
		bool matches(clang::FunctionDecl* fdecl) const override { return fdecl->isExternC(); }
	};

	Mode default_mode = Mode::Process;
	std::vector<std::pair<std::unique_ptr<Filter>, Mode>> filters;

	static std::optional<Mode> parse_default(rapidjson::Value const& v) {
		if(!v.IsObject()) {
			return {};
		}

		Mode result = Mode::Process;
		for(auto const& member : v.GetObject()) {
			if(member.name == "mode") {
				if(auto mode = parse_mode({member.value.GetString(), member.value.GetStringLength()})) {
					result = *mode;
				} else {
					return {};
				}
			} else {
				return {};
			}
		}
		return {std::move(result)};
	}

	static std::optional<std::pair<std::unique_ptr<Filter>, Mode>> parse_override(rapidjson::Value const& v) {
		if(!v.IsObject()) {
			return {};
		}

		bool mode_set = false;
		std::pair<std::unique_ptr<Filter>, Mode> result;
		for(auto const& member : v.GetObject()) {
			if(member.name == "mode") {
				if(auto mode = parse_mode({member.value.GetString(), member.value.GetStringLength()})) {
					result.second = *mode;
					mode_set = true;
				} else {
					return {};
				}
			} else if(member.name == "name") {
				result.first = Filter::combine(
				  std::move(result.first),
				  std::make_unique<NameFilter>(std::string{member.value.GetString(), member.value.GetStringLength()}));
			} else if(member.name == "linkage") {
				if(member.value == "inline") {
					result.first = Filter::combine(std::move(result.first), std::make_unique<InlineFilter>());
				} else if(member.value == "extern") {
					result.first = Filter::combine(std::move(result.first), std::make_unique<ExternFilter>());
				} else {
					return {};
				}
			} else {
				return {};
			}
		}

		if(mode_set) {
			return {std::move(result)};
		} else {
			return {};
		}
	}

public:
	static std::optional<FunctionFilter> from_file(std::string const& filter_path) {
		if(std::ifstream ifs{filter_path}) {
			FunctionFilter result;

			rapidjson::IStreamWrapper isw{ifs};
			rapidjson::Document doc;
			doc.ParseStream(isw);

			if(!doc.IsObject()) {
				return {};
			}

			for(auto const& member : doc.GetObject()) {
				if(member.name == "default") {
					if(auto t = parse_default(member.value)) {
						result.default_mode = std::move(*t);
					} else {
						return {};
					}
				} else if(member.name == "overrides") {
					if(!member.value.IsArray()) {
						return {};
					}

					for(auto const& override_spec : member.value.GetArray()) {
						if(auto f = parse_override(override_spec)) {
							result.filters.push_back(std::move(*f));
						} else {
							return {};
						}
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

	bool skip(clang::FunctionDecl* fdecl) const {
		for(auto const& [filter, mode] : filters) {
			if(filter->matches(fdecl)) {
				return mode == Mode::Skip;
			}
		}

		return default_mode == Mode::Skip;
	}
};

inline std::unique_ptr<FunctionFilter::Filter>
FunctionFilter::Filter::combine(std::unique_ptr<FunctionFilter::Filter> lhs,
                                std::unique_ptr<FunctionFilter::Filter> rhs) {
	if(!lhs) {
		return rhs;
	}
	if(!rhs) {
		return lhs;
	}

	if(auto* multi = dynamic_cast<FunctionFilter::MultiFilter*>(lhs.get())) {
		multi->add_filter(std::move(rhs));
		return lhs;
	}
	if(auto* multi = dynamic_cast<FunctionFilter::MultiFilter*>(rhs.get())) {
		multi->add_filter(std::move(lhs));
		return rhs;
	}

	auto result = std::make_unique<FunctionFilter::MultiFilter>();
	result->add_filter(std::move(lhs));
	result->add_filter(std::move(rhs));
	return result;
}
