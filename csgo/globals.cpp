#include "globals.h"
#include "sdk/sdk.h"

vec3_t g::last_real = vec3_t( 0.0f, 0.0f, 0.0f );
vec3_t g::last_fake = vec3_t( 0.0f, 0.0f, 0.0f );
ucmd_t* g::ucmd = nullptr;
ucmd_t g::raw_ucmd;
bool g::send_packet = true;
bool* g::psend_packet = nullptr;