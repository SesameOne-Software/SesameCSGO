#pragma once
#include "detours.h"
#include "patternscanner.h"
#include "vfunc.h"
#include "padding.h"
#include "../cheat.h"

namespace utils {
	extern FILE* console;

	void log( const wchar_t* msg, ... );
}