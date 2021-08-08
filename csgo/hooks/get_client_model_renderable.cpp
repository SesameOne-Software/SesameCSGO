#include "get_client_model_renderable.hpp"

decltype( &hooks::get_client_model_renderable ) hooks::old::get_client_model_renderable = nullptr;

void* __fastcall hooks::get_client_model_renderable ( REG ) {
	return nullptr;
}