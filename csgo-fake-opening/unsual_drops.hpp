#pragma once
#include <map>
#include <vector>
#include <cinttypes>

struct skin_drop_info {
	int32_t def_index;
	int32_t paint_kit;
};

namespace unsual_drops {
	extern std::map<int32_t, std::vector<skin_drop_info>> crates_drop_unsual;
};

