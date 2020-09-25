#include <Windows.h>

#include "csgo_fake_opening.hpp"

BOOL APIENTRY DllMain( HMODULE h_module,
    DWORD  ul_reason_for_call,
    LPVOID lp_reserved
) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( h_module );
        csgo_fake_opening::my_module = h_module;
        CreateThread( NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>( csgo_fake_opening::init ), NULL, 0, NULL );
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

