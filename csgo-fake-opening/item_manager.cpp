#include "item_manager.hpp"
#include "csgo_sdk.hpp"
#include "fnv_hash.hpp"
#include "sdk.hpp"

#include <Windows.h>

void item_manager::init( ) {
	auto item_schema = csgo_sdk::get_item_schema( );

	for ( size_t i = 0; i < item_schema->get_item_definition_count( ); i++ ) {
		auto item = item_schema->get_item_definition( i );
		auto type_hash = fnv::hash_runtime( item->get_item_type( ) );

		switch ( type_hash ) {
			case FNV( "#CSGO_Type_WeaponCase" ):
				if ( strstr( item->get_weapon_name( ), "coupon " ) != item->get_weapon_name( ) )
					crates.push_back( item );
				break;
			case FNV( "#CSGO_Tool_WeaponCase_KeyTag" ):
				keys.push_back( item );
				break;
		}
	}

	char name[ 256 ];
	static const auto V_UCS2ToUTF8 = reinterpret_cast<int( * )( const wchar_t* ucs2, char* utf8, int len )>( GetProcAddress( GetModuleHandle( L"vstdlib.dll" ), "V_UCS2ToUTF8" ) );

	for ( auto i = 1;; i++ ) {
		std::string event_name = "CSGO_Tournament_Event_Stage_Display_" + std::to_string( i );
		const auto wide_name = sdk::localize->find( event_name.c_str( ) );
		if ( !wide_name )
			break;
		V_UCS2ToUTF8( wide_name, name, sizeof( name ) );
		tournament_event_stages.push_back( name );
	}

	int32_t max_team_id = -1;
	std::vector<int32_t> players_ids;
	for ( size_t i = 0; i < item_schema->get_sticker_definition_count( ); i++ ) {
		auto item = item_schema->get_sticker_definition( i );
		if ( item->get_team_id( ) > max_team_id )
			max_team_id = item->get_team_id( );

		if ( item->get_player_id( ) > 0 )
			if ( std::find( players_ids.begin( ), players_ids.end( ), item->get_player_id( ) ) == players_ids.end( ) )
				players_ids.push_back( item->get_player_id( ) );

		if ( strstr( item->get_name( ), "#SprayKit" ) == item->get_name( ) )
			continue;

		stickers.push_back( item );
	}

	for ( auto i = 1; i <= max_team_id; i++ ) {
		std::string team_name = "CSGO_TeamID_" + std::to_string( i );
		const auto wide_name = sdk::localize->find( team_name.c_str( ) );
		if ( !wide_name ) {
			csgo_teams.push_back( "REMOVED" );
			continue;
		}
		V_UCS2ToUTF8( wide_name, name, sizeof( name ) );
		csgo_teams.push_back( name );
	}

	for ( auto i : players_ids ) {
		auto player_data = csgo_sdk::get_item_schema( )->get_pro_player_data( i );
		if ( !player_data )
			continue;

		std::string player_name = "#SFUI_ProPlayer_" + std::string( player_data->get_player_nick( ) );
		const auto wide_name = sdk::localize->find( player_name.c_str( ) );
		if ( !wide_name )
			continue;

		V_UCS2ToUTF8( wide_name, name, sizeof( name ) );
		csgo_players[ i ] = std::string(name) + " (" + std::string( player_data->get_player_nick( ) ) + ")";
	}
}

namespace item_manager {
	std::vector<econ_item_definition*> crates;
	std::vector<econ_item_definition*> keys;
	std::vector<econ_sticker_definition*> stickers;
	std::vector<std::string> tournament_event_stages;
	std::vector<std::string> csgo_teams;
	std::map<int32_t, std::string> csgo_players;
};