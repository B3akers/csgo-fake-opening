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
#include "hooks.hpp"
#include "vmt_smart_hook.hpp"
#include "sdk.hpp"
#include "utils.hpp"
#include "unsual_drops.hpp"
#include "item_manager.hpp"
#include "menu.hpp"

#include <memory>
#include <string>
#include <intrin.h>
#include <random>

c_econ_item_view* opened_crate = nullptr;

namespace inventory_manager_vtable {
	struct set_item_backpack_position {
		static bool __fastcall hooked( void* p_this, void*, c_econ_item_view* item, int a1, int a2, bool a3 ) {
			if ( item->get_inventory( ) == 0 ) {
				item->get_inventory( ) = sdk::inventory_manager->get_local_player_inventory( )->get_last_values_ids( ).second + 1;
				item->get_soc_data( )->get_inventory( ) = item->get_inventory( );
				sdk::ui_engine_source->send_panorama_component_my_persona_inventory_updated( );
			}
			return m_original( p_this, nullptr, item, a1, a2, a3 );
		}
		static decltype( &hooked ) m_original;
	};
	decltype( set_item_backpack_position::m_original ) set_item_backpack_position::m_original;
}

namespace ui_engine_source_vtable {
	struct broadcast_event {
		static bool __fastcall hooked( void* p_this, void*, uintptr_t event ) {
			static const auto failed_to_open_crate = *reinterpret_cast<uintptr_t*>( utils::pattren_scan( "client.dll", "68 ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0 84 C0 74 1A 8B 35 ? ? ? ? 8B D3 33 C9 8B 3E E8 ? ? ? ? 50 8B CE FF 97 ? ? ? ? 5F 5E B0 01 5B 8B E5 5D C2 04 00" ) + 1 );

			if ( failed_to_open_crate == event && opened_crate ) {
				static const auto item_customization_notification = *reinterpret_cast<uintptr_t*>( utils::pattren_scan( "client.dll", "68 ? ? ? ? 8B 80 ? ? ? ? FF D0 84 C0 74 28" ) + 1 );
				auto create_series = opened_crate->get_crate_series( );
				if ( create_series && sdk::ui_engine_source->broadcast_event( item_customization_notification ) ) {
					auto inventory = sdk::inventory_manager->get_local_player_inventory( );

					auto last_ids = inventory->get_last_values_ids( );
					auto item = c_econ_item::create_econ_item( );
					item->get_account_id( ) = inventory->get_steam_id( );
					item->get_item_id( ) = last_ids.first + 1;
					item->get_inventory( ) = 0;
					item->get_flags( ) = 0;
					item->get_original_id( ) = 0;

					std::map<int32_t, std::vector<weapon_drop_info>> items_map;

					auto crate_items = csgo_sdk::get_weapons_for_crate( create_series );
					auto unsual_drops = unsual_drops::crates_drop_unsual.find( opened_crate->get_static_data( )->get_definition_index( ) );
					auto has_unsual_drops = unsual_drops != unsual_drops::crates_drop_unsual.end( );
					if ( has_unsual_drops )
						for ( auto& item : unsual_drops->second )
							crate_items.push_back( { item.def_index,item.paint_kit,7,0 } );

					for ( auto& item : crate_items )
						items_map[ item.rarity ].push_back( item );

					std::random_device rd;
					std::mt19937 gen( rd( ) );
					std::uniform_real_distribution<> dis( 0, 100 );
					std::uniform_real_distribution<> seed( 0, 1000 );
					auto chance = dis( gen );
					auto pow = std::pow( 100. / fakeopening_chances::knife_chance, 1. / static_cast<double>( items_map.size( ) - 1 ) );
					auto dropped = items_map.begin( )->second.front( );
					auto count = 0;

					for ( auto it = items_map.rbegin( ); it != items_map.rend( ); ++it ) {
						double percent = fakeopening_chances::knife_chance;
						for ( auto i = 0; i < count; i++ )
							percent *= pow;

						if ( chance <= percent ) {
							std::uniform_int_distribution<> dis( 0, it->second.size( ) - 1 );
							dropped = it->second.at( dis( gen ) );
							break;
						}

						count++;
					}

					item->get_def_index( ) = dropped.item_def;

					if ( dropped.paintkit > 0 ) {
						item->set_paint_kit( static_cast<float>( dropped.paintkit ) );
						item->set_paint_seed( static_cast<float>( seed( gen ) ) );

						auto wear_chance = dis( gen );
						auto wear = 0.f;

						if ( wear_chance < fakeopening_chances::factory_new_chance )
							wear = static_cast<float>( std::uniform_real_distribution<>( 0, 0.07 )( gen ) );
						else {
							auto wear_random = std::uniform_int_distribution<>( 0, 2 )( gen );
							switch ( wear_random ) {
								case 0:
									wear = static_cast<float>( std::uniform_real_distribution<>( 0, 1 )( gen ) );
									break;
								case 1:
									wear = static_cast<float>( std::uniform_real_distribution<>( 0, 0.44 )( gen ) );
									break;
								case 2:
									wear = static_cast<float>( std::uniform_real_distribution<>( 0, 0.15 )( gen ) );
									break;
							}
						}
						item->set_paint_wear( wear );
					} else if ( dropped.sticker_id > 0 )
						item->set_attribute_value( 113, &dropped.sticker_id );

					item->set_quality( ITEM_QUALITY_SKIN );
					item->set_rarity( static_cast<item_rarity>( dropped.rarity ) );

					auto attributes_crate = opened_crate->get_soc_data( )->get_attributes( );
					auto is_tournament_crate = std::find( attributes_crate.begin( ), attributes_crate.end( ), 137 ) != attributes_crate.end( );

					if ( dropped.sticker_id == 0 ) {
						if ( !is_tournament_crate ) {
							//Make sure item isn't glove
							//
							if ( item->get_def_index( ) < 5000 ) {
								auto stat_track_chance = dis( gen );
								if ( stat_track_chance < fakeopening_chances::statstark_chance ) {
									item->set_stat_trak( 0 );
									item->set_quality( ITEM_QUALITY_STRANGE );
								}
							}
						} else {
							int32_t tournament_id = -1, team0 = -1, team1 = -1, stage = -1, player_mvp = -1;
							for ( auto& attr : attributes_crate ) {
								auto value = *reinterpret_cast<int32_t*>( &attr );
								switch ( attr.id ) {
									case 137:
										tournament_id = value;
										break;
									case 138:
										stage = value;
										break;
									case 139:
										team0 = value;
										break;
									case 140:
										team1 = value;
										break;
									case 223:
										player_mvp = value;
										break;
								}
							}

							item->set_attribute_value( 137, &tournament_id );
							item->set_attribute_value( 138, &stage );
							item->set_attribute_value( 139, &team0 );
							item->set_attribute_value( 140, &team1 );
							if ( player_mvp > 0 )
								item->set_attribute_value( 223, &player_mvp );

							if ( tournament_id != -1 && team0 != -1 && team1 != -1 ) {
								std::vector<econ_sticker_definition*> player_stickers;
								int32_t tournament_sticker = 0, team0sticker = 0, team1sticker = 0, playersticker = 0;

								for ( auto& sticker : item_manager::stickers ) {
									if ( sticker->get_event_id( ) == tournament_id && ( strstr( sticker->get_name( ), "_gold" ) || player_mvp == -1 ) ) {
										if ( sticker->get_team_id( ) == 0 && sticker->get_player_id( ) == 0 )
											tournament_sticker = sticker->get_sticker_id( );
										else if ( sticker->get_team_id( ) == team0 && sticker->get_player_id( ) == 0 )
											team0sticker = sticker->get_sticker_id( );
										else if ( sticker->get_team_id( ) == team1 && sticker->get_player_id( ) == 0 )
											team1sticker = sticker->get_sticker_id( );
										else if ( sticker->get_player_id( ) > 0 && sticker->get_player_id( ) == player_mvp )
											playersticker = sticker->get_sticker_id( );
									}
								}

								if ( tournament_sticker != 0 )
									item->add_sticker( 0, tournament_sticker, 0, 1, 0 );

								if ( team0sticker != 0 )
									item->add_sticker( 1, team0sticker, 0, 1, 0 );

								if ( team1sticker != 0 )
									item->add_sticker( 2, team1sticker, 0, 1, 0 );

								if ( playersticker != 0 )
									item->add_sticker( 3, playersticker, 0, 1, 0 );

							}

							item->set_quality( ITEM_QUALITY_TOURNAMENT );
						}
					}

					if ( dropped.rarity == 7 && has_unsual_drops ) {
						item->set_quality( ITEM_QUALITY_UNUSUAL );
						item->set_rarity( ITEM_RARITY_ANCIENT );
					}

					item->set_origin( 8 );
					item->set_level( 1 );
					item->set_in_use( false );

					inventory->add_econ_item( item, 1, 0, 1 );

					if ( !opened_crate->get_static_data( )->get_associated_items( ).empty( ) ) {
						auto key = inventory->find_key_to_open( opened_crate );
						if ( key )
							inventory->remove_item( key );
					}

					inventory->remove_item( opened_crate );

					static auto init_item_customization_notification_event
						= reinterpret_cast<uintptr_t( __fastcall* )( void*, int, const char*, const char* )>(
							utils::pattren_scan( "client.dll", "55 8B EC A1 ? ? ? ? 53 56 8B F1 8B DA 8B 08 57 6A 1C 8B 01 FF 50 04 8B F8 85 FF 74 48" )
							);

					auto event = init_item_customization_notification_event( 0, 0, "crate_unlock", std::to_string( item->get_item_id( ) ).c_str( ) );

					sdk::ui_engine_source->dispatch_event( event );
				}
			}

			return m_original( p_this, nullptr, event );
		}
		static decltype( &hooked ) m_original;
	};
	decltype( broadcast_event::m_original ) broadcast_event::m_original;
}

namespace panorama_marshall_helper_vtable {
	struct unknowncall {
		static const char* __fastcall hooked( void* p_this, void*, int a1, int a2 ) {
			auto ret = m_original( p_this, nullptr, a1, a2 );;

			static auto get_loot_list_items_count_return_address = reinterpret_cast<uintptr_t>( utils::pattren_scan( "client.dll", "85 C0 0F 84 ? ? ? ? 8B C8 E8 ? ? ? ? 52 50 E8 ? ? ? ? 83 C4 08 89 44 24 14 85 C0 0F 84 ? ? ? ? 8B 0D" ) );
			if ( reinterpret_cast<uintptr_t>( _ReturnAddress( ) ) == get_loot_list_items_count_return_address ) {
				opened_crate = sdk::inventory_manager->get_local_player_inventory( )->get_inventory_item_by_item_id( std::stoull( ret ) );
			}

			return ret;
		}
		static decltype( &hooked ) m_original;
	};
	decltype( unknowncall::m_original ) unknowncall::m_original;
};

std::unique_ptr<vmt_smart_hook> inventory_manager_vmt = nullptr;
std::unique_ptr<vmt_smart_hook> ui_engine_source_vmt = nullptr;
std::unique_ptr<vmt_smart_hook> panorama_marshall_helper_vmt = nullptr;

void hooks::hook( ) {
	inventory_manager_vmt = std::make_unique<vmt_smart_hook>( sdk::inventory_manager );
	inventory_manager_vmt->apply_hook<inventory_manager_vtable::set_item_backpack_position>( 26 );

	ui_engine_source_vmt = std::make_unique<vmt_smart_hook>( sdk::ui_engine_source );
	ui_engine_source_vmt->apply_hook<ui_engine_source_vtable::broadcast_event>( 54 );

	panorama_marshall_helper_vmt = std::make_unique<vmt_smart_hook>( sdk::panorama_marshall_helper );
	panorama_marshall_helper_vmt->apply_hook<panorama_marshall_helper_vtable::unknowncall>( 7 );
}
void hooks::unhook( ) {
	inventory_manager_vmt->unhook( );
	ui_engine_source_vmt->unhook( );
	panorama_marshall_helper_vmt->unhook( );
}
