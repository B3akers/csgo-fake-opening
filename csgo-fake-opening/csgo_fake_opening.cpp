#include "csgo_fake_opening.hpp"
#include "sdk.hpp"
#include "d3d_hook.hpp"
#include "item_manager.hpp"
#include "hooks.hpp"

#include <thread>
#include <chrono>

void csgo_fake_opening::init( ) {
	AllocConsole( );
	freopen( "CONIN$", "r", stdin );
	freopen( "CONOUT$", "w", stdout );
	freopen( "CONOUT$", "w", stderr );

	sdk::init( );
	hooks::hook( );
	item_manager::init( );
	d3d_hook::hook( );

	while ( true ) {

		if ( GetAsyncKeyState( VK_F7 ) )
			break;

		using namespace std::chrono_literals;
		std::this_thread::sleep_for( 200ms );
	}

	hooks::unhook( );
	d3d_hook::unhook( );

	FreeConsole( );

	FreeLibraryAndExitThread( my_module, 0 );
}

HMODULE csgo_fake_opening::my_module;