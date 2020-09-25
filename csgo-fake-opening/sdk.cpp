#include "sdk.hpp"
#include "utils.hpp"

#include <stdio.h>

create_interface_fn get_module_factory( HMODULE module ) {
    return reinterpret_cast<create_interface_fn>( GetProcAddress( module, "CreateInterface" ) );
}

template<typename T>
T* get_interface( create_interface_fn f, const char* szInterfaceVersion ) {
    return reinterpret_cast<T*>( f( szInterfaceVersion, nullptr ) );
}

void sdk::init( ) {
	auto input_sys_factory = get_module_factory( GetModuleHandleW( L"inputsystem.dll" ) );
	auto localize_sys_factory = get_module_factory( GetModuleHandleW( L"localize.dll" ) );

	input_system = get_interface<i_input_system>( input_sys_factory, "InputSystemVersion001" );
	localize = get_interface<i_localize>( localize_sys_factory, "Localize_001" );

	panorama_marshall_helper = *reinterpret_cast<uintptr_t**>( utils::pattren_scan( "client.dll", "68 ? ? ? ? 8B C8 E8 ? ? ? ? 8D 4D F4 FF 15 ? ? ? ? 8B CF FF 15 ? ? ? ? 5F 5E 8B E5 5D C3" ) + 1 );
	inventory_manager = *reinterpret_cast<cs_inventory_manager**>( utils::pattren_scan( "client.dll", "B9 ?? ?? ?? ?? 8D 44 24 10 89 54 24 14" ) + 0x1 );
	gc_client = **reinterpret_cast<gc_client_system***>( utils::pattren_scan( "client.dll", "8B 0D ? ? ? ? 6A 00 83 EC 10" ) + 0x2 );
	ui_engine_source = **reinterpret_cast<ui_engine_source2***>( utils::pattren_scan( "client.dll", "8B 35 ? ? ? ? 8B D3 33 C9 8B 3E E8 ? ? ? ? 50 8B CE FF 97 ? ? ? ? 5F 5E B0 01 5B 8B E5 5D C2 04 00" ) + 0x2 );
	g_d3d_device = **(IDirect3DDevice9***)( utils::pattren_scan( "shaderapidx9.dll", "A1 ? ? ? ? 50 8B 08 FF 51 0C" ) + 1 );

	auto params = D3DDEVICE_CREATION_PARAMETERS { 0 };
	g_d3d_device->GetCreationParameters( &params );
	window = params.hFocusWindow;
}

namespace sdk {
	i_input_system* input_system = nullptr;
	i_localize* localize = nullptr;
	cs_inventory_manager* inventory_manager = nullptr;
	gc_client_system* gc_client = nullptr;
	ui_engine_source2* ui_engine_source = nullptr;
	uintptr_t* panorama_marshall_helper = nullptr;

	IDirect3DDevice9* g_d3d_device = nullptr;
	HWND window;
};