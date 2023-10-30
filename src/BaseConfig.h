#pragma once

#include <rapidjson/document.h>

#include <optional>
#include <type_traits>

struct BaseConfig {
	bool enabled;

	BaseConfig(bool enabledByDefault = true)
	  : enabled(enabledByDefault) { }

	template <typename DerivedConfig, typename F>
	static std::optional<DerivedConfig> parse(rapidjson::Value const& v, F member_callback) {
		static_assert(std::is_base_of_v<BaseConfig, DerivedConfig>);

		DerivedConfig result{};

		if(v.IsBool()) {
			result.enabled = v.GetBool();
		} else if(v.IsObject()) {
			for(auto const& member : v.GetObject()) {
				if(!member_callback(result, member)) {
					return {};
				}
			}
		}

		return {std::move(result)};
	}
};