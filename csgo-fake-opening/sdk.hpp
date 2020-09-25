#pragma once
#include <Windows.h>
#include <d3d9.h>

#include "csgo_sdk.hpp"

typedef void* ( *create_interface_fn )( const char* name, int* return_code );

namespace sdk {
	void init( );

	extern i_input_system* input_system;
	extern i_localize* localize;
	extern cs_inventory_manager* inventory_manager;
	extern gc_client_system* gc_client;
	extern ui_engine_source2* ui_engine_source;
	extern uintptr_t* panorama_marshall_helper;


	extern IDirect3DDevice9* g_d3d_device;
	extern HWND window;
};

