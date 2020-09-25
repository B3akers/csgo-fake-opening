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
#pragma once
#include <cinttypes>
#include <vector>
#include <utility>

#include "CUtlVector.hpp"

enum item_quality {
	ITEM_QUALITY_DEFAULT,
	ITEM_QUALITY_GENUINE,
	ITEM_QUALITY_VINTAGE,
	ITEM_QUALITY_UNUSUAL,
	ITEM_QUALITY_SKIN,
	ITEM_QUALITY_COMMUNITY,
	ITEM_QUALITY_DEVELOPER,
	ITEM_QUALITY_SELFMADE,
	ITEM_QUALITY_CUSTOMIZED,
	ITEM_QUALITY_STRANGE,
	ITEM_QUALITY_COMPLETED,
	ITEM_QUALITY_SKIN_STRANGE,
	ITEM_QUALITY_TOURNAMENT
};

enum item_rarity {
	ITEM_RARITY_DEFAULT,
	ITEM_RARITY_COMMON,
	ITEM_RARITY_UNCOMMON,
	ITEM_RARITY_RARE,
	ITEM_RARITY_MYTHICAL,
	ITEM_RARITY_LEGENDARY,
	ITEM_RARITY_ANCIENT,
	ITEM_RARITY_IMMORTAL
};

template <size_t Index, typename ReturnType, typename... Args>
__forceinline ReturnType call_virtual( void* instance, Args... args ) {
	using Fn = ReturnType( __thiscall* )( void*, Args... );

	auto function = ( *reinterpret_cast<Fn**>( instance ) )[ Index ];
	return function( instance, args... );
}

class gc_client_system {
public:
};

class i_input_system {
public:
	void enable_input( bool enable ) {
		return call_virtual<11, void>( this, enable );
	}
};

class i_localize {
public:
	wchar_t* find( const char* token_name ) {
		return call_virtual<11, wchar_t*>( this, token_name );
	}
};

struct attribute_info {
	int16_t id;
	int32_t value;

	bool operator ==( const int16_t& other ) { return id == other; }
};

class econ_sticker_definition {
public:
	int32_t get_team_id( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) + 0x6C ); }
	int32_t get_rarity( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) + 0x4 ); }
	int32_t get_event_id( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) + 0x68 ); }
	int32_t get_player_id( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) + 0x70 ); }
	int32_t get_sticker_id( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) ); }

	const char* get_name( ) { return *reinterpret_cast<const char**>( std::uintptr_t( this ) + 0x28 ); }
};

class econ_paint_kit_definition {
public:
	int32_t get_paint_kit( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) ); }
	int32_t get_rarity_value( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) + 0x68 ); }
};

class econ_item_definition {
public:
	int32_t get_definition_index( ) { return *reinterpret_cast<int32_t*>( std::uintptr_t( this ) + 0x8 ); }
	int32_t get_rarity_value( ) { return *reinterpret_cast<char*>( std::uintptr_t( this ) + 0x2A ); }
	const char* get_item_type( ) { return *reinterpret_cast<const char**>( std::uintptr_t( this ) + 0x54 ); }
	const char* get_weapon_name( ) { return *reinterpret_cast<const char**>( std::uintptr_t( this ) + 0x1BC ); }
	const wchar_t* get_weapon_localize_name( );
	std::vector<attribute_info> get_attributes( );
	std::vector<uint16_t> get_associated_items( );
};

class pro_player_data {
public:
	const char* get_player_nick( ) { return *reinterpret_cast<const char**>( std::uintptr_t( this ) + 0x18 ); }
};

class item_schema {
public:
	size_t get_item_definition_count( ) { return *reinterpret_cast<size_t*>( std::uintptr_t( this ) + 0xE8 ); }
	econ_item_definition* get_item_definition( size_t i ) { return *reinterpret_cast<econ_item_definition**>( *reinterpret_cast<uintptr_t*>( std::uintptr_t( this ) + 0xD4 ) + 4 * ( 3 * i ) + 4 ); }

	size_t get_paint_kit_definition_count( ) { return *reinterpret_cast<size_t*>( std::uintptr_t( this ) + 0x2A8 ); }
	econ_paint_kit_definition* get_paint_kit_definition( size_t i ) { return *reinterpret_cast<econ_paint_kit_definition**>( *reinterpret_cast<uintptr_t*>( std::uintptr_t( this ) + 0x290 ) + 24 * i + 20 ); }

	size_t get_sticker_definition_count( ) { return *reinterpret_cast<size_t*>( std::uintptr_t( this ) + 0x2CC ); }
	econ_sticker_definition* get_sticker_definition( size_t i ) { return *reinterpret_cast<econ_sticker_definition**>( *reinterpret_cast<uintptr_t*>( std::uintptr_t( this ) + 0x2B4 ) + 24 * i + 0x14 ); }

	uintptr_t get_attribute_by_index( size_t index ) { return *reinterpret_cast<uintptr_t*>( *reinterpret_cast<uintptr_t*>( reinterpret_cast<uintptr_t>( this ) + 0x120 ) + 4 * index ); }

	econ_item_definition* get_item_by_definition_index( int32_t index );
	econ_paint_kit_definition* get_paint_kit_by_skin_index( int32_t index );
	econ_sticker_definition* get_sticker_by_skin_index( int32_t index );

	const char* get_create_series_by_id( int32_t index );

	pro_player_data* get_pro_player_data( int32_t index );
};

struct econ_attirbute {
	char data[ 16 ];
	int32_t size;
	int32_t type;
	int32_t id;

	bool operator ==( const int32_t& other ) { return id == other; }
};

class c_econ_item {
public:
	static c_econ_item* create_econ_item( );
	static void destroy( c_econ_item* item );

	uint32_t& get_inventory( );
	uint32_t& get_account_id( );
	uint16_t& get_def_index( );
	uint64_t& get_item_id( );
	uint64_t& get_original_id( );

	unsigned short& get_econ_item_data( );

	unsigned char& get_flags( );

	void set_quality( item_quality quality );
	void set_rarity( item_rarity rarity );
	void set_origin( int origin );
	void set_level( int level );
	void set_in_use( bool in_use );

	void set_paint_kit( float kit );
	void set_paint_seed( float seed );
	void set_paint_wear( float wear );
	void set_stat_trak( int val );
	void add_sticker( int index, int kit, float wear, float scale, float rotation );

	void set_attribute_value( int index, void* val );

	void update_equipped_state( unsigned int state );

	std::vector<unsigned int> get_equipped_state( );
	std::vector<econ_attirbute> get_attributes( );
};

class ui_engine_source2 {
public:
	void send_panorama_component_my_persona_inventory_updated( );

	void dispatch_event( uintptr_t event ) {
		return call_virtual<52, void>( this, event );
	}

	bool broadcast_event( uintptr_t event ) {
		return call_virtual<54, bool>( this, event );
	}
};

class c_econ_item_view {
public:
	void clear_inventory_image_rgba( );
	uint32_t& get_inventory( );
	c_econ_item* get_soc_data( );
	econ_item_definition* get_static_data( );
	const char* get_crate_series( );
	bool tool_can_apply_to( c_econ_item_view* item );
};

class c_shared_object_type_cache {
public:
	void add_object( void* obj ) {
		return call_virtual<1, void>( this, obj );
	}
	void remove_object( void* obj ) {
		return call_virtual<3, void>( this, obj );
	}

	std::vector<c_econ_item*> get_econ_items( );
};

class c_player_inventory {
public:
	c_econ_item_view* find_key_to_open( c_econ_item_view* crate );
	c_econ_item_view* get_inventory_item_by_item_id( int64_t id );
	c_shared_object_type_cache* get_base_type_cache( );
	bool add_econ_item( c_econ_item* item, int a3, int a4, char a5 );
	std::pair<uint64_t, uint32_t> get_last_values_ids( );
	void remove_item( c_econ_item_view* item );
	void remove_item( uint64_t id );

	uint32_t get_steam_id( ) { return *reinterpret_cast<uint32_t*>( std::uintptr_t( this ) + 0x8 ); }
	CUtlVector< c_econ_item_view* >* get_inventory_items( ) { return reinterpret_cast<CUtlVector<c_econ_item_view*>*>( std::uintptr_t( this ) + 0x2C ); }
};

class cs_inventory_manager {
public:
	c_player_inventory* get_local_player_inventory( );
};

struct weapon_drop_info {
	int32_t item_def;
	int32_t paintkit;
	int32_t rarity;
	int32_t sticker_id;
};

namespace csgo_sdk {
	item_schema* get_item_schema( );
	std::vector<weapon_drop_info> get_weapons_for_crate( const char* name );
};
