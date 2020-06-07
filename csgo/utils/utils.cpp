#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <windows.h>
#include "utils.hpp"

std::array< bool, 5 > mouse_down { false };
std::array< bool, 512 > key_down { false };