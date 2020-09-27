/* This file is part of csgo-fake-opening by B3akers, licensed under the MIT license:
*
* MIT License
*
* Copyright (c) b3akers 2020
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "menu.hpp"
#include "item_manager.hpp"
#include "csgo_sdk.hpp"
#include "sdk.hpp"

#include "../imgui/imgui.h"

#include <Windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>

std::once_flag init_lists;

struct menu_item_econ_item {
	econ_item_definition* item;
	std::string name;
};

struct list_econ_item {
	std::vector<econ_item_definition*>* original_list;
	int current_item = 0;
	std::vector<char> filter_buffer;
	std::vector<menu_item_econ_item> items_filtered;

	void apply_filter_items( ) {
		static const auto V_UCS2ToUTF8 = reinterpret_cast<int( * )( const wchar_t* ucs2, char* utf8, int len )>( GetProcAddress( GetModuleHandle( L"vstdlib.dll" ), "V_UCS2ToUTF8" ) ); /* nSkinz namazso*/

		items_filtered.clear( );

		auto filter = std::string( filter_buffer.data( ) );
		std::transform( filter.begin( ), filter.end( ), filter.begin( ), ::tolower );

		std::vector<menu_item_econ_item> best_results, possible_results;

		for ( auto& item : *original_list ) {
			auto wide_name = item->get_weapon_localize_name( );
			char name[ 256 ];
			V_UCS2ToUTF8( wide_name, name, sizeof( name ) );

			if ( filter.empty( ) )
				best_results.push_back( { item, name } );
			else {
				auto str_name = std::string( name );
				std::transform( str_name.begin( ), str_name.end( ), str_name.begin( ), ::tolower );
				auto pos = str_name.find( filter );
				if ( pos == 0 )
					best_results.push_back( { item, name } );
				else if ( pos != std::string::npos )
					possible_results.push_back( { item, name } );
			}
		}

		items_filtered.reserve( best_results.size( ) + possible_results.size( ) );
		items_filtered.insert( items_filtered.end( ), best_results.begin( ), best_results.end( ) );
		items_filtered.insert( items_filtered.end( ), possible_results.begin( ), possible_results.end( ) );

		if ( current_item >= static_cast<int>( items_filtered.size( ) ) )
			current_item = 0;
	}


	void init( std::vector<econ_item_definition*>& list ) {
		original_list = &list;
		filter_buffer.resize( 256 );
		apply_filter_items( );
	}
};


//item-lists
list_econ_item crate_list;
list_econ_item key_list;

//crate-create-option
auto crate_as_unacknowledged = false;
auto add_key_to_crate = false;
auto crate_create_size = 1;
auto stage_tournament_crate = 0;
auto team0_tournament_crate = 0;
auto team1_tournament_crate = 0;
auto player_tournament_crate = 0;

//key-crate-option
auto key_as_unacknowledged = false;
auto key_create_size = 1;

//teams
std::vector<std::pair<int32_t, std::string>> csgo_teams_filtered;
std::vector<std::pair<int32_t, std::string>> csgo_players_filtered;

//tabs
auto current_tab = 0;

//fakeopneing
float fakeopening_chances::knife_chance = 0.2f;
float fakeopening_chances::statstark_chance = 10.f;
float fakeopening_chances::factory_new_chance = 14.f;

void update_players( int32_t tournament_id ) {
	csgo_players_filtered.clear( );
	csgo_players_filtered.push_back( { 0, "None" } );

	if ( tournament_id > 0 ) {
		auto t0 = csgo_teams_filtered[ team0_tournament_crate ].first;
		auto t1 = csgo_teams_filtered[ team1_tournament_crate ].first;
		std::vector<int32_t> players;
		for ( auto& sticker : item_manager::stickers ) {
			if ( sticker->get_event_id( ) == tournament_id ) {
				if ( sticker->get_player_id( ) > 0 && (sticker->get_team_id() == t0 || sticker->get_team_id() == t1))
					if ( std::find( players.begin( ), players.end( ), sticker->get_player_id( ) ) == players.end( ) )
						players.push_back( sticker->get_player_id( ) );
			}
		}

		for ( auto id : players )
			csgo_players_filtered.push_back( { id, item_manager::csgo_players[ id ] } );
	}

	if ( player_tournament_crate >= static_cast<int32_t>( csgo_players_filtered.size( ) ) )
		player_tournament_crate = 0;
}

void update_teams( ) {
	csgo_teams_filtered.clear( );

	auto tournament_id_val = 0;

	if ( crate_list.current_item < static_cast<int32_t>( crate_list.items_filtered.size( ) ) ) {
		auto& crate_item = crate_list.items_filtered.at( crate_list.current_item );
		auto attributes_crate = crate_item.item->get_attributes( );
		auto is_tournament_crate = true;
		auto supply_create_series = std::find( attributes_crate.begin( ), attributes_crate.end( ), 68 );
		if ( supply_create_series != attributes_crate.end( ) ) {
			auto name = csgo_sdk::get_item_schema( )->get_create_series_by_id( supply_create_series->value );
			if ( name ) {
				auto items = csgo_sdk::get_weapons_for_crate( name );
				if ( items.front( ).sticker_id != 0 )
					is_tournament_crate = false;
			}
		}

		if ( is_tournament_crate ) {
			auto tournament_id = std::find( attributes_crate.begin( ), attributes_crate.end( ), 137 );
			if ( tournament_id != attributes_crate.end( ) ) {
				std::vector<int32_t> teams;
				for ( auto& sticker : item_manager::stickers ) {
					if ( sticker->get_event_id( ) == tournament_id->value ) {

						if ( sticker->get_team_id( ) > 0 )
							if ( std::find( teams.begin( ), teams.end( ), sticker->get_team_id( ) ) == teams.end( ) )
								teams.push_back( sticker->get_team_id( ) );
					}
				}

				for ( auto id : teams )
					csgo_teams_filtered.push_back( { id, item_manager::csgo_teams[ id - 1 ] } );

				tournament_id_val = tournament_id->value;
			}
		}
	}

	if ( team0_tournament_crate >= static_cast<int32_t>( csgo_teams_filtered.size( ) ) )
		team0_tournament_crate = 0;

	if ( team1_tournament_crate >= static_cast<int32_t>( csgo_teams_filtered.size( ) ) )
		team1_tournament_crate = 0;

	update_players( tournament_id_val );
}

void menu::draw( ) {
	std::call_once( init_lists, [ & ] ( ) {
		crate_list.init( item_manager::crates );
		key_list.init( item_manager::keys );
		} );

	static auto vector_getter_item_definition = [ ] ( void* vec, int idx, const char** out_text ) {
		auto& vector = *static_cast<std::vector<menu_item_econ_item>*>( vec );
		if ( idx < 0 || idx >= static_cast<int>( vector.size( ) ) ) { return false; }
		*out_text = vector.at( idx ).name.c_str( );
		return true;
	};

	static auto vector_getter = [ ] ( void* vec, int idx, const char** out_text ) {
		auto& vector = *static_cast<std::vector<std::string>*>( vec );
		if ( idx < 0 || idx >= static_cast<int>( vector.size( ) ) ) { return false; }
		*out_text = vector.at( idx ).c_str( );
		return true;
	};

	static auto vector_getter_pair = [ ] ( void* vec, int idx, const char** out_text ) {
		auto& vector = *static_cast<std::vector<std::pair<int32_t, std::string>>*>( vec );
		if ( idx < 0 || idx >= static_cast<int>( vector.size( ) ) ) { return false; }
		*out_text = vector.at( idx ).second.c_str( );
		return true;
	};

	ImGui::Begin( "csgo-fake-opening" );
	{
		if ( ImGui::Button( "Crate/key creator", ImVec2( 350, 30 ) ) )
			current_tab = 0;

		ImGui::SameLine( );

		if ( ImGui::Button( "Fakeopening chances", ImVec2( 350, 30 ) ) )
			current_tab = 1;

		ImGui::Separator( );

		if ( current_tab == 0 ) {
			ImGui::Text( "=== Crate creator" );

			if ( ImGui::InputText( "Filter##crate", crate_list.filter_buffer.data( ), crate_list.filter_buffer.size( ) ) ) {
				crate_list.apply_filter_items( );
				update_teams( );
			}

			if ( ImGui::ListBox( "Crates", &crate_list.current_item, vector_getter_item_definition, static_cast<void*>( &crate_list.items_filtered ), crate_list.items_filtered.size( ) ) ) {
				update_teams( );
			}
			
			ImGui::Separator( );

			if ( crate_list.current_item < (int)crate_list.items_filtered.size( ) ) {
				auto& crate_item = crate_list.items_filtered.at( crate_list.current_item );
				auto attributes_crate = crate_item.item->get_attributes( );
				auto tournament_id = std::find( attributes_crate.begin( ), attributes_crate.end( ), 137);
				auto is_tournament_crate = tournament_id != attributes_crate.end( ) && !csgo_teams_filtered.empty( );

				ImGui::Checkbox( "Add key to open this crate", &add_key_to_crate );
				ImGui::SliderInt( "Count", &crate_create_size, 1, 50 );
				ImGui::Checkbox( "Add crate/key as unacknowledged", &crate_as_unacknowledged );

				if ( is_tournament_crate ) {
					ImGui::ListBox( "Stage", &stage_tournament_crate, vector_getter, static_cast<void*>( &item_manager::tournament_event_stages ), item_manager::tournament_event_stages.size( ) ); ImGui::Separator( );
					if ( ImGui::ListBox( "Team 0", &team0_tournament_crate, vector_getter_pair, static_cast<void*>( &csgo_teams_filtered ), csgo_teams_filtered.size( ) ) ) { update_players( tournament_id->value ); } ImGui::Separator( );
					if ( ImGui::ListBox( "Team 1", &team1_tournament_crate, vector_getter_pair, static_cast<void*>( &csgo_teams_filtered ), csgo_teams_filtered.size( ) ) ) { update_players( tournament_id->value ); } ImGui::Separator( );
					ImGui::ListBox( "Mvp player", &player_tournament_crate, vector_getter_pair, static_cast<void*>( &csgo_players_filtered ), csgo_players_filtered.size( ) ); ImGui::Separator( );
				}

				if ( ImGui::Button( "Create & add to inventory", ImVec2( 200, 40 ) ) ) {
					auto inventory = sdk::inventory_manager->get_local_player_inventory( );

					for ( auto i = 0; i < crate_create_size; i++ ) {
						auto last_ids = inventory->get_last_values_ids( );
						auto item = c_econ_item::create_econ_item( );
						item->get_account_id( ) = inventory->get_steam_id( );
						item->get_def_index( ) = crate_item.item->get_definition_index( );
						item->get_item_id( ) = last_ids.first + 1;
						item->get_inventory( ) = crate_as_unacknowledged ? 0 : last_ids.second + 1;
						item->get_flags( ) = 0;
						item->get_original_id( ) = 0;

						item->set_origin( 8 );
						item->set_level( 1 );
						item->set_in_use( false );

						item->set_quality( ITEM_QUALITY_SKIN );
						item->set_rarity( ITEM_RARITY_DEFAULT );

						if ( is_tournament_crate ) {
							auto s = stage_tournament_crate + 1;
							auto t0 = csgo_teams_filtered[ team0_tournament_crate ].first;
							auto t1 = csgo_teams_filtered[ team1_tournament_crate ].first;
							auto p = csgo_players_filtered[ player_tournament_crate ].first;
							item->set_attribute_value( 138, &s );
							item->set_attribute_value( 139, &t0 );
							item->set_attribute_value( 140, &t1 );
							if ( p > 0 )
								item->set_attribute_value( 223, &p );
						}

						for ( auto& attr : attributes_crate )
							item->set_attribute_value( attr.id, &attr.value );

						inventory->add_econ_item( item, 1, 0, 1 );
					}

					if ( add_key_to_crate ) {
						auto key_id = crate_item.item->get_associated_items( ).empty( ) ? -1 : crate_item.item->get_associated_items( ).back( );
						if ( key_id != -1 ) {
							for ( auto i = 0; i < crate_create_size; i++ ) {
								auto last_ids = inventory->get_last_values_ids( );
								auto item = c_econ_item::create_econ_item( );
								item->get_account_id( ) = inventory->get_steam_id( );
								item->get_def_index( ) = key_id;
								item->get_item_id( ) = last_ids.first + 1;
								item->get_inventory( ) = crate_as_unacknowledged ? 0 : last_ids.second + 1;
								item->get_flags( ) = 0;
								item->get_original_id( ) = 0;

								item->set_origin( 8 );
								item->set_level( 1 );
								item->set_in_use( false );

								item->set_quality( ITEM_QUALITY_SKIN );
								item->set_rarity( ITEM_RARITY_DEFAULT );

								inventory->add_econ_item( item, 1, 0, 1 );
							}
						}
					}
				}
			}

			ImGui::Text( "=== Key creator" );

			if ( ImGui::InputText( "Filter##keys", key_list.filter_buffer.data( ), key_list.filter_buffer.size( ) ) )
				key_list.apply_filter_items( );
			ImGui::ListBox( "Keys", &key_list.current_item, vector_getter_item_definition, static_cast<void*>( &key_list.items_filtered ), key_list.items_filtered.size( ) );

			if ( key_list.current_item < (int)key_list.items_filtered.size( ) ) {
				auto& key_item = key_list.items_filtered.at( key_list.current_item );

				ImGui::SliderInt( "Count##key", &key_create_size, 1, 50 );
				ImGui::Checkbox( "Add key as unacknowledged", &key_as_unacknowledged );
				if ( ImGui::Button( "Create & add to inventory##key", ImVec2( 200, 40 ) ) ) {
					auto inventory = sdk::inventory_manager->get_local_player_inventory( );
					for ( auto i = 0; i < key_create_size; i++ ) {
						auto last_ids = inventory->get_last_values_ids( );
						auto item = c_econ_item::create_econ_item( );
						item->get_account_id( ) = inventory->get_steam_id( );
						item->get_def_index( ) = key_item.item->get_definition_index( );
						item->get_item_id( ) = last_ids.first + 1;
						item->get_inventory( ) = key_as_unacknowledged ? 0 : last_ids.second + 1;
						item->get_flags( ) = 0;
						item->get_original_id( ) = 0;

						item->set_origin( 8 );
						item->set_level( 1 );
						item->set_in_use( false );

						item->set_quality( ITEM_QUALITY_SKIN );
						item->set_rarity( ITEM_RARITY_DEFAULT );

						inventory->add_econ_item( item, 1, 0, 1 );
					}
				}
			}
		} else if ( current_tab == 1 ) {
			ImGui::SliderFloat( "Knife chance (more = better items)", &fakeopening_chances::knife_chance, 0.f, 100.f );
			ImGui::SliderFloat( "Stattrak chance", &fakeopening_chances::statstark_chance, 0.f, 100.f );
			ImGui::SliderFloat( "Factory new wear chance", &fakeopening_chances::factory_new_chance, 0.f, 100.f );
		}

		ImGui::End( );
	};
}
