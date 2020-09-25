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
#include "csgo_sdk.hpp"
#include "sdk.hpp"
#include "utils.hpp"

#include <string>

//csgo_sdk
item_schema* csgo_sdk::get_item_schema( ) {
	static auto fn_get_item_schema
		= reinterpret_cast<item_schema * ( __stdcall* )( )>(
			utils::pattren_scan( "client.dll", "A1 ? ? ? ? 85 C0 75 53" )
			);
	return fn_get_item_schema( );
}

//item_schema
econ_item_definition* item_schema::get_item_by_definition_index( int32_t index ) {
	for ( size_t i = 0; i < get_item_definition_count( ); i++ ) {
		auto item = get_item_definition( i );
		if ( item->get_definition_index( ) == index )
			return item;
	}
	return nullptr;
}

econ_paint_kit_definition* item_schema::get_paint_kit_by_skin_index( int32_t index ) {
	for ( size_t i = 0; i < get_paint_kit_definition_count( ); i++ ) {
		auto item = get_paint_kit_definition( i );
		if ( item->get_paint_kit( ) == index )
			return item;
	}
	return nullptr;
}

econ_sticker_definition* item_schema::get_sticker_by_skin_index( int32_t index ) {
	for ( size_t i = 0; i < get_sticker_definition_count( ); i++ ) {
		auto item = get_sticker_definition( i );
		if ( item->get_sticker_id( ) == index )
			return item;
	}
	return nullptr;
}

pro_player_data* item_schema::get_pro_player_data( int32_t index ) {
	static auto address = utils::pattren_scan( "client.dll", "E8 ? ? ? ? 68 ? ? ? ? 8B F0 8D 44 24 7C" );

	static auto fn_get_player_data
		= reinterpret_cast<pro_player_data * ( __thiscall* )( uintptr_t, int32_t )>(
			*reinterpret_cast<uintptr_t*>( address + 1 ) + address + 5
			);
	return fn_get_player_data( std::uintptr_t( this ) + 4, index );
}


const char* item_schema::get_create_series_by_id( int32_t index ) {
	static auto fnUnk
		= reinterpret_cast<int( __thiscall* )( uintptr_t, int* )>(
			utils::pattren_scan( "client.dll", "55 8B EC 8B 45 08 56 57 8B 30 8B 41 10 83 F8 FF 74 1E 8B 79 04 8D 0C 40 8B 54 CF 10 3B D6 7E 05" )
			);

	auto ID = fnUnk( (uintptr_t)this + 0x17C, &index );

	if ( ID == -1 )
		return nullptr;

	auto v11 = *(DWORD*)( (uintptr_t)this + 0x17C + 4 ) + 24 * ID;

	return *reinterpret_cast<const char**>( v11 + 0x14 );
}

void recursive_add_loot_to_loot_list( uintptr_t v2, std::vector<weapon_drop_info>& drop ) {
	auto size = *(DWORD*)( ( *( int( __thiscall** )( int ) )( *(DWORD*)v2 + 4 ) )( v2 ) + 12 );
	auto v9 = 0;
	auto v8 = 0u;
	do {
		auto v4 = (DWORD*)( *( int( __thiscall** )( int ) )( *(DWORD*)v2 + 4 ) )( v2 );
		auto v5 = v9 + *v4;

		if ( *(BYTE*)( v5 + 24 ) ) {
			static auto fnGetLootListInterfaceByIndex
				= reinterpret_cast<uintptr_t( __thiscall* )( uintptr_t, uintptr_t )>(
					utils::pattren_scan( "client.dll", "55 8B EC 8B 55 08 56 8B F1 85 D2 78 47" )
					);
			auto v7 = fnGetLootListInterfaceByIndex( std::uintptr_t( csgo_sdk::get_item_schema( ) ) + 4, *(DWORD*)v5 );
			recursive_add_loot_to_loot_list( v7, drop );
		} else {
			auto paintkit = *reinterpret_cast<int*>( v5 + 0x4 );
			auto stickerkit = *reinterpret_cast<int*>( v5 + 0x10 );
			auto itemdef = *reinterpret_cast<int*>( v5 );
			if ( itemdef != 0 ) {
				auto rarity = 0;

				if ( stickerkit != 0 )
					rarity = csgo_sdk::get_item_schema( )->get_sticker_by_skin_index( stickerkit )->get_rarity( );
				else {
					auto item_rarity = csgo_sdk::get_item_schema( )->get_item_by_definition_index( itemdef )->get_rarity_value( );
					auto skin_rarity = paintkit == 0 ? 0 : csgo_sdk::get_item_schema( )->get_paint_kit_by_skin_index( paintkit )->get_rarity_value( );

					auto skin_rarity_fixed = ( skin_rarity == 7 ) + 6;
					rarity = item_rarity + skin_rarity - 1;

					if ( rarity >= 0 ) {
						if ( rarity > skin_rarity_fixed )
							rarity = skin_rarity_fixed;
					} else
						rarity = 0;
				}

				drop.push_back( { itemdef, paintkit, rarity, stickerkit } );
			}
		}

		v9 += 28;
		++v8;
	} while ( v8 < size );
}

std::vector<weapon_drop_info> csgo_sdk::get_weapons_for_crate( const char* name ) {
	std::vector<weapon_drop_info> drop;

	static auto fn_get_loot_list_by_name
		= reinterpret_cast<uintptr_t( __thiscall* )( uintptr_t, const char*, signed int )>(
			utils::pattren_scan( "client.dll", "55 8B EC 83 E4 F8 8B 55 08 81 EC ? ? ? ? 56 57" )
			);

	recursive_add_loot_to_loot_list( fn_get_loot_list_by_name( std::uintptr_t( get_item_schema( ) ) + 4, name, 0 ), drop );
	return drop;
}

//econ_item_definition
const wchar_t* econ_item_definition::get_weapon_localize_name( ) {
	return sdk::localize->find( *reinterpret_cast<const char**>( std::uintptr_t( this ) + 0x4C ) );
}

std::vector<attribute_info> econ_item_definition::get_attributes( ) {
	std::vector<attribute_info> attributes;

	auto size = *reinterpret_cast<int*>( std::uintptr_t( this ) + 0x3C );
	auto data = *reinterpret_cast<uintptr_t*>( std::uintptr_t( this ) + 0x30 );

	if ( data ) {
		for ( int i = 0; i < size; i++ ) {
			auto id = *reinterpret_cast<int16_t*>( data + ( i * 0xC ) );
			auto value = *reinterpret_cast<int32_t*>( data + ( i * 0xC ) + 0x4 );
			attributes.push_back( { id,value } );
		}
	}

	return attributes;
}

std::vector<uint16_t> econ_item_definition::get_associated_items( ) {
	std::vector<uint16_t> items;

	auto size = *reinterpret_cast<int*>( std::uintptr_t( this ) + 0x18 );
	auto data = *reinterpret_cast<uintptr_t*>( std::uintptr_t( this ) + 0xC );

	if ( data ) {
		for ( int i = 0; i < size; i++ )
			items.push_back( *reinterpret_cast<uint16_t*>( data + ( i * sizeof( uint16_t ) ) ) );
	}

	return items;
}

//cs_inventory_manager
c_player_inventory* cs_inventory_manager::get_local_player_inventory( ) {
	static auto local_inventory_offset = *reinterpret_cast<uintptr_t*>( utils::pattren_scan( "client.dll", "8B 8F ? ? ? ? 0F B7 C0 50" ) + 0x2 );

	return *reinterpret_cast<c_player_inventory**>( this + local_inventory_offset );
}

//c_player_inventory
bool c_player_inventory::add_econ_item( c_econ_item* item, int a3, int a4, char a5 ) {
	static auto fn_add_econ_item
		= reinterpret_cast<bool( __thiscall* )( void*, c_econ_item*, int, int, char )>(
			utils::pattren_scan( "client.dll", "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 14 53 56 57 8B F9 8B 08" )
			);

	get_base_type_cache( )->add_object( item );

	if ( !fn_add_econ_item( this, item, a3, a4, a5 ) )
		return false;

	auto i = get_inventory_item_by_item_id( item->get_item_id( ) );

	if ( !i )
		return false;

	i->clear_inventory_image_rgba( );

	return true;
}

std::pair<uint64_t, uint32_t> c_player_inventory::get_last_values_ids( ) {
	uint64_t max_id = 1;
	uint32_t max_inv_id = 1;
	for ( auto&& i : get_base_type_cache( )->get_econ_items( ) ) {
		auto is_default_item = (uint32_t)( i->get_item_id( ) >> 32 ) == 0xF0000000;
		if ( !is_default_item ) {
			if ( i->get_item_id( ) > max_id )
				max_id = i->get_item_id( );

			if ( i->get_inventory( ) > max_inv_id )
				max_inv_id = i->get_inventory( );
		}
	}

	return { max_id, max_inv_id };
}

c_econ_item_view* c_player_inventory::get_inventory_item_by_item_id( int64_t id ) {
	static auto fngetinventoryitembyitemid
		= reinterpret_cast<c_econ_item_view * ( __thiscall* )( void*, int64_t )>(
			utils::pattren_scan( "client.dll", "55 8B EC 8B 55 08 83 EC 10 8B C2" )
			);
	return fngetinventoryitembyitemid( this, id );
}

c_shared_object_type_cache* c_player_inventory::get_base_type_cache( ) {
	static auto fn_gcsdk_cgcclient_findsocache
		= reinterpret_cast<uintptr_t( __thiscall* )( uintptr_t, uint64_t, uint64_t, bool )>(
			utils::pattren_scan( "client.dll", "55 8B EC 83 E4 F8 83 EC 1C 0F 10 45 08" )
			);

	static auto fngcsdk_csharedobjectcache_createbasetypecache
		= reinterpret_cast<c_shared_object_type_cache * ( __thiscall* )( uintptr_t, int )>(
			utils::pattren_scan( "client.dll", "55 8B EC 51 53 56 8B D9 8D 45 08" )
			);

	auto so_cahce = fn_gcsdk_cgcclient_findsocache( reinterpret_cast<uintptr_t>( sdk::gc_client ) + 0x70, *reinterpret_cast<uint64_t*>( this + 0x8 ), *reinterpret_cast<uint64_t*>( this + 0x10 ), 1 );

	return fngcsdk_csharedobjectcache_createbasetypecache( so_cahce, 1 );
}

void c_player_inventory::remove_item( uint64_t id ) {
	static auto fn_remove_item
		= reinterpret_cast<int( __thiscall* )( void*, int64_t )>(
			utils::pattren_scan( "client.dll", "55 8B EC 83 E4 F8 51 53 56 57 FF 75 0C 8B F1" )
			);

	fn_remove_item( this, id );
}

void c_player_inventory::remove_item( c_econ_item_view* item ) {
	auto econ_item = item->get_soc_data( );

	remove_item( econ_item->get_item_id() );
	get_base_type_cache( )->remove_object( econ_item );

	c_econ_item::destroy( econ_item );

	sdk::ui_engine_source->send_panorama_component_my_persona_inventory_updated( );
}

c_econ_item_view* c_player_inventory::find_key_to_open( c_econ_item_view* crate ) {
	for ( int i = 0; i < this->get_inventory_items( )->GetSize( ); i++ ) {
		auto prop_key = *( this->get_inventory_items( )->GetMemory( ).OffsetBufferByIndex( i ) );
		if ( prop_key ) {
			if ( prop_key->tool_can_apply_to( crate ) )
				return prop_key;
		}
	}

	return nullptr;
}

//c_econ_item_view
void c_econ_item_view::clear_inventory_image_rgba( ) {
	static auto fnclearinventoryimagergba
		= reinterpret_cast<int* ( __thiscall* )( void* )>(
			utils::pattren_scan( "client.dll", "55 8B EC 81 EC ? ? ? ? 57 8B F9 C7 47" )
			);
	fnclearinventoryimagergba( this );
}

uint32_t& c_econ_item_view::get_inventory( ) {
	static auto inv_offset = *reinterpret_cast<uintptr_t*>( utils::pattren_scan( "client.dll", "8D 9E ? ? ? ? 8B 0B" ) + 2 );
	return *reinterpret_cast<uint32_t*>( (uintptr_t)this + inv_offset );
}

c_econ_item* c_econ_item_view::get_soc_data( ) {
	static auto fn_get_soc_data
		= reinterpret_cast<c_econ_item * ( __thiscall* )( c_econ_item_view* )>(
			utils::pattren_scan( "client.dll", "55 8B EC 83 E4 F0 83 EC 18 56 8B F1 57 8B 86" )
			);

	return fn_get_soc_data( this );
}

econ_item_definition* c_econ_item_view::get_static_data( ) {
	static auto fngetstaticdata
		= reinterpret_cast<econ_item_definition * ( __thiscall* )( void* )>(
			utils::pattren_scan( "client.dll", "55 8B EC 51 53 8B D9 8B 0D ? ? ? ? 56 57 8B B9" )
			);
	return fngetstaticdata( this );
}

const char* c_econ_item_view::get_crate_series( ) {
	auto attributes_crate = this->get_static_data( )->get_attributes( );

	auto supply_create_series = std::find( attributes_crate.begin( ), attributes_crate.end( ), 68 );
	if ( supply_create_series != attributes_crate.end( ) )
		return csgo_sdk::get_item_schema( )->get_create_series_by_id( supply_create_series->value );

	return nullptr;
}

bool c_econ_item_view::tool_can_apply_to( c_econ_item_view* item ) {
	static auto fn_tool_can_apply_to = utils::pattren_scan( "client.dll", "55 8B EC 83 EC 18 53 56 8B F1 57 8B FA" );
	bool ret_val;

	__asm
	{
		mov eax, this
		add eax, 0xC
		mov ecx, eax
		mov eax, item
		add eax, 0xC
		mov edx, eax
		push 0x4
		call fn_tool_can_apply_to
		mov ret_val, al
		add esp, 4
	};

	return ret_val;
}

//c_econ_item
c_econ_item* c_econ_item::create_econ_item( ) {
	static auto fncreatesharedobjectsubclass_econitem_
		= reinterpret_cast<c_econ_item * ( __stdcall* )( )>(
			*reinterpret_cast<uintptr_t*>( utils::pattren_scan( "client.dll", "C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4 C7 45 ? ? ? ? ? 50 C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4 C7 45 ? ? ? ? ? 50 C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4 C7 45 ? ? ? ? ? 50 C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? E8 ? ? ? ? 83 F8 FF 75 09 8D 45 E4 50 E8 ? ? ? ? 8D 45 E4" ) + 3 )
			);

	return fncreatesharedobjectsubclass_econitem_( );
}

void c_econ_item::destroy( c_econ_item* item ) {
	static auto fn_econ_item
		= reinterpret_cast<int( __thiscall* )( void*, bool )>(
			utils::pattren_scan( "client.dll", "55 8B EC 56 8B F1 8B 4E 18 C7 06 ? ? ? ? C7 46 ? ? ? ? ? 85 C9 74 05" )
			);
	fn_econ_item( item, true );
}

uint32_t& c_econ_item::get_inventory( ) {
	return *reinterpret_cast<uint32_t*>( reinterpret_cast<uintptr_t>( this ) + 0x20 );
}

uint32_t& c_econ_item::get_account_id( ) {
	return *reinterpret_cast<uint32_t*>( reinterpret_cast<uintptr_t>( this ) + 0x1C );
}

uint16_t& c_econ_item::get_def_index( ) {
	return *reinterpret_cast<uint16_t*>( reinterpret_cast<uintptr_t>( this ) + 0x24 );
}

uint64_t& c_econ_item::get_item_id( ) {
	return *reinterpret_cast<uint64_t*>( reinterpret_cast<uintptr_t>( this ) + 0x8 );
}

uint64_t& c_econ_item::get_original_id( ) {
	return *reinterpret_cast<uint64_t*>( reinterpret_cast<uintptr_t>( this ) + 0x10 );
}

unsigned char& c_econ_item::get_flags( ) {
	return *reinterpret_cast<unsigned char*>( reinterpret_cast<uintptr_t>( this ) + 0x30 );
}

unsigned short& c_econ_item::get_econ_item_data( ) {
	return *reinterpret_cast<unsigned short*>( reinterpret_cast<uintptr_t>( this ) + 0x26 );
}

void c_econ_item::set_quality( item_quality quality ) {
	auto data = get_econ_item_data( );
	get_econ_item_data( ) = data ^ ( data ^ 32 * quality ) & 0x1E0;
}

void c_econ_item::set_rarity( item_rarity rarity ) {
	auto data = get_econ_item_data( );
	get_econ_item_data( ) = ( data ^ ( rarity << 11 ) ) & 0x7800 ^ data;
}

void c_econ_item::set_origin( int origin ) {
	auto data = get_econ_item_data( );
	get_econ_item_data( ) = data ^ ( (unsigned __int8)data ^ (unsigned __int8)origin ) & 0x1F;
}

void c_econ_item::set_level( int level ) {
	auto data = get_econ_item_data( );
	get_econ_item_data( ) = data ^ ( data ^ ( level << 9 ) ) & 0x600;
}

void c_econ_item::set_in_use( bool in_use ) {
	auto data = get_econ_item_data( );
	get_econ_item_data( ) = data & 0x7FFF | ( in_use << 15 );
}

void c_econ_item::set_attribute_value( int index, void* val ) {
	auto attrib = csgo_sdk::get_item_schema( )->get_attribute_by_index( index );

	static auto fnsetdynamicattributevalue
		= reinterpret_cast<int( __thiscall* )( c_econ_item*, uintptr_t, void* )>(
			utils::pattren_scan( "client.dll", "55 8B EC 83 E4 F8 83 EC 3C 53 8B 5D 08 56 57 6A 00" )
			);

	fnsetdynamicattributevalue( this, attrib, val );
}

void c_econ_item::update_equipped_state( unsigned int state ) {
	static auto fn_update_equipped_state
		= reinterpret_cast<void( __thiscall* )( c_econ_item*, unsigned int )>(
			utils::pattren_scan( "client.dll", "55 8B EC 8B 45 08 8B D0 C1 EA 10" )
			);

	return fn_update_equipped_state( this, state );
}

void c_econ_item::set_paint_kit( float kit ) {
	set_attribute_value( 6, &kit );
}

void c_econ_item::set_paint_seed( float seed ) {
	set_attribute_value( 7, &seed );
}

void c_econ_item::set_stat_trak( int val ) {
	int32_t zero = 0;
	set_attribute_value( 80, &val );
	set_attribute_value( 81, &zero );
}

void c_econ_item::set_paint_wear( float wear ) {
	set_attribute_value( 8, &wear );
}

void c_econ_item::add_sticker( int index, int kit, float wear, float scale, float rotation ) {
	set_attribute_value( 113 + 4 * index, &kit );
	set_attribute_value( 114 + 4 * index, &wear );
	set_attribute_value( 115 + 4 * index, &scale );
	set_attribute_value( 116 + 4 * index, &rotation );
}

std::vector<unsigned int> c_econ_item::get_equipped_state( ) {
	auto eq_st = *reinterpret_cast<uintptr_t*>( (uintptr_t)this + 0x18 );
	std::vector<unsigned int> EquippedState;

	if ( eq_st ) {
		if ( ( **(BYTE**)( (uintptr_t)this + 0x18 ) & 0x3F ) != 63 ) {
			auto v31 = **(WORD**)( (uintptr_t)this + 0x18 );
			unsigned short v32 = ( v31 >> 6 ) & 7; //2
			unsigned short v33 = v31 & 0x3F; //3

			unsigned int final = ( ( v33 << 16 ) | ( v32 ) );

			EquippedState.push_back( final );

			if ( **(WORD**)( (uintptr_t)this + 24 ) & 0x200 && ( **(WORD**)( (uintptr_t)this + 24 ) & 0x1C0 ) == 128 ) {
				unsigned short v1 = 3;
				unsigned short v2 = v31 & 0x3F; //3

				unsigned int final2 = ( ( v2 << 16 ) | ( v1 ) );

				EquippedState.push_back( final2 );
			}
		}
	}

	return EquippedState;
}

std::vector<econ_attirbute> c_econ_item::get_attributes( ) {
	std::vector<econ_attirbute> attributes;

	auto v16 = *(WORD**)( std::uintptr_t( this ) + 0x18 );
	if ( v16 ) {
		auto v17 = v16 + 1;
		auto v48 = (unsigned int)&v16[ 4 * ( (unsigned int)*v16 >> 10 ) + 1 ];
		if ( (unsigned int)( v16 + 1 ) < v48 ) {
			do {
				auto v18 = (DWORD*)csgo_sdk::get_item_schema( );
				auto v19 = *v17;
				if ( v19 < v18[ 75 ] ) {
					auto v20 = *(DWORD*)( v18[ 72 ] + 4 * v19 );
					if ( v20 ) {
						auto attrib = econ_attirbute( );
						attrib.id = v19;

						auto attrib_pointer = &attrib;

						*(DWORD*)( (uintptr_t)attrib_pointer + 20 ) = 15;
						*(DWORD*)( (uintptr_t)attrib_pointer + 16 ) = 0;
						*(BYTE*)(uintptr_t)attrib_pointer = 0;

						auto v45 = *(DWORD*)( v20 + 0xC );

						( *( void( __thiscall** )( DWORD, int, DWORD ) )( *(DWORD*)v45 + 12 ) )( v45, (int)( v17 + 2 ), (uintptr_t)attrib_pointer );

						attributes.push_back( attrib );
					}
				}

				v17 += 4;
			} while ( (unsigned int)v17 < v48 );
		}
	}

	return attributes;
}

//c_shared_object_type_cache
std::vector<c_econ_item*> c_shared_object_type_cache::get_econ_items( ) {
	std::vector<c_econ_item*> ret;

	auto size = *reinterpret_cast<size_t*>( std::uintptr_t( this ) + 0x18 );

	auto data = *reinterpret_cast<uintptr_t**>( std::uintptr_t( this ) + 0x4 );

	for ( size_t i = 0; i < size; i++ )
		ret.push_back( reinterpret_cast<c_econ_item*>( data[ i ] ) );

	return ret;
}

//ui_engine_source2
void ui_engine_source2::send_panorama_component_my_persona_inventory_updated( ) {
	static auto address = utils::pattren_scan( "client.dll", "E8 ? ? ? ? 83 C4 04 80 BF ? ? ? ? ? 74 0C" );

	static auto fn_send
		= reinterpret_cast<int( __stdcall* )( )>(
			*reinterpret_cast<uintptr_t*>( address + 1 ) + address + 5
			);

	fn_send( );
}