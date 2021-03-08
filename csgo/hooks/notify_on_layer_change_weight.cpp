#include "notify_on_layer_change_weight.hpp"

decltype( &hooks::notify_on_layer_change_weight ) hooks::old::notify_on_layer_change_weight = nullptr;

void __fastcall hooks::notify_on_layer_change_weight( REG , const animlayer_t* layer , const float new_weight ) {

}