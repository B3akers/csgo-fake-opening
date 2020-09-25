#pragma once
#include <cinttypes>
namespace utils {
	std::uint8_t* pattren_scan( const char* module, const char* signature );
	std::uint8_t* pattren_scan( void* module, const char* signature );
};