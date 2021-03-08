#include "notify_on_layer_change_cycle.hpp"

decltype( &hooks::notify_on_layer_change_cycle ) hooks::old::notify_on_layer_change_cycle = nullptr;

void __fastcall hooks::notify_on_layer_change_cycle( REG , const animlayer_t* layer , const float new_cycle ) {

}