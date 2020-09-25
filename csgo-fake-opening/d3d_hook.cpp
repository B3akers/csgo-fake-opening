#include "d3d_hook.hpp"
#include "sdk.hpp"
#include "menu.hpp"

#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include "vmt_smart_hook.hpp"

#include <thread>
#include <mutex>
#include <memory>
#include <intrin.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

std::once_flag init_device;
std::unique_ptr<vmt_smart_hook> d3d_device_vmt = nullptr;

bool get_system_font_path( const std::string& name, std::string& path ) {
	char buffer[ MAX_PATH ];
	HKEY registryKey;

	GetWindowsDirectoryA( buffer, MAX_PATH );
	std::string fontsFolder = buffer + std::string( "\\Fonts\\" );

	if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &registryKey ) ) {
		return false;
	}

	uint32_t valueIndex = 0;
	char valueName[ MAX_PATH ];
	uint8_t valueData[ MAX_PATH ];
	std::wstring wsFontFile;

	do {
		uint32_t valueNameSize = MAX_PATH;
		uint32_t valueDataSize = MAX_PATH;
		uint32_t valueType;

		auto error = RegEnumValueA(
			registryKey,
			valueIndex,
			valueName,
			reinterpret_cast<DWORD*>( &valueNameSize ),
			0,
			reinterpret_cast<DWORD*>( &valueType ),
			valueData,
			reinterpret_cast<DWORD*>( &valueDataSize ) );

		valueIndex++;

		if ( error == ERROR_NO_MORE_ITEMS ) {
			RegCloseKey( registryKey );
			return false;
		}

		if ( error || valueType != REG_SZ ) {
			continue;
		}

		if ( _strnicmp( name.data( ), valueName, name.size( ) ) == 0 ) {
			path = fontsFolder + std::string( (char*)valueData, valueDataSize );
			RegCloseKey( registryKey );
			return true;
		}
	} while ( true );

	return false;
}

static const ImWchar ranges[ ] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0x0400, 0x044F, // Cyrillic
	0x0100, 0x017F, // Latin Extended-A
	0x0180, 0x024F, // Latin Extended-B
	0x2000, 0x206F, // General Punctuation
	0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
	0x31F0, 0x31FF, // Katakana Phonetic Extensions
	0xFF00, 0xFFEF, // Half-width characters
	0x4e00, 0x9FAF, // CJK Ideograms
	0,
};

bool menu_is_open = false;

LONG_PTR org_wnd_proc;
LRESULT __stdcall wnd_proc( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) {
	if ( ImGui_ImplWin32_WndProcHandler( hwnd, msg, w_param, l_param ) )
		return true;

	if ( msg == WM_KEYDOWN && w_param == VK_INSERT ) {
		sdk::input_system->enable_input( menu_is_open );
		menu_is_open = !menu_is_open;
	}

	return CallWindowProc( (WNDPROC)org_wnd_proc, hwnd, msg, w_param, l_param );
}

namespace d3d_vtable {
	struct end_scene {
		static long __stdcall hooked( IDirect3DDevice9* p_device ) {

			// https://www.unknowncheats.me/forum/counterstrike-global-offensive/276438-stream-proof-visuals-method.html
			//
			static uintptr_t gameoverlay_return_address = 0;
			if ( !gameoverlay_return_address ) {
				MEMORY_BASIC_INFORMATION info;
				VirtualQuery( _ReturnAddress( ), &info, sizeof( MEMORY_BASIC_INFORMATION ) );

				char mod[ MAX_PATH ];
				GetModuleFileNameA( (HMODULE)info.AllocationBase, mod, MAX_PATH );

				if ( strstr( mod,  "gameoverlay"  ) )
					gameoverlay_return_address = (uintptr_t)( _ReturnAddress( ) );
			}

			if ( gameoverlay_return_address != (uintptr_t)( _ReturnAddress( ) ) )
				return m_original( p_device );

			std::call_once( init_device, [ & ] ( ) {

				ImGui::CreateContext( );
				ImGui::StyleColorsDark( );

				ImGui::GetIO( ).ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

				std::string font_path;
				if ( get_system_font_path( "Courier", font_path ) )
					ImGui::GetIO( ).Fonts->AddFontFromFileTTF( font_path.c_str( ), 14, 0, ranges );

				ImGui_ImplWin32_Init( sdk::window );
				ImGui_ImplDX9_Init( p_device );

				org_wnd_proc = SetWindowLongPtr( sdk::window, GWLP_WNDPROC, (LONG_PTR)wnd_proc );
				} );

			if ( menu_is_open ) {
				ImGui_ImplDX9_NewFrame( );
				ImGui_ImplWin32_NewFrame( );
				ImGui::NewFrame( );

				menu::draw( );

				ImGui::EndFrame( );
				ImGui::Render( );
				ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData( ) );
			}

			return m_original( p_device );
		}
		static decltype( &hooked ) m_original;
	};
	decltype( end_scene::m_original ) end_scene::m_original;

	struct reset {
		static long __stdcall hooked( IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parametrs ) {

			ImGui_ImplDX9_InvalidateDeviceObjects( );

			auto hr = m_original( device, parametrs );

			if ( hr >= 0 )
				ImGui_ImplDX9_CreateDeviceObjects( );

			return hr;

		}
		static decltype( &hooked ) m_original;
	};
	decltype( reset::m_original ) reset::m_original;
};
using namespace d3d_vtable;

void d3d_hook::hook( ) {
	d3d_device_vmt = std::make_unique<::vmt_smart_hook>( sdk::g_d3d_device );
	d3d_device_vmt->apply_hook<end_scene>( 42 );
	d3d_device_vmt->apply_hook<reset>( 16 );
}

void d3d_hook::unhook( ) {
	SetWindowLongPtr( sdk::window, GWLP_WNDPROC, org_wnd_proc );
	d3d_device_vmt->unhook( );
}