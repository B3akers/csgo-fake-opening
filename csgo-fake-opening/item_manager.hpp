#pragma once
#include <vector>
#include <string>
#include <map>

class econ_item_definition;
class econ_sticker_definition;

namespace item_manager {
	void init( );

	extern std::vector<econ_item_definition*> crates;
	extern std::vector<econ_item_definition*> keys;
	extern std::vector<econ_sticker_definition*> stickers;

	extern std::vector<std::string> tournament_event_stages;
	extern std::vector<std::string> csgo_teams;
	extern std::map<int32_t, std::string> csgo_players;
};

