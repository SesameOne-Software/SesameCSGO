#pragma once
#include "../utils/utils.hpp"

class network_string_table_t;
class c_network_string_table_container;

class network_string_table_t {
public:
    int add_string ( bool is_server, const char* value, int length = -1, const void* userdata = nullptr ) {
        using fn = int( __thiscall* )( network_string_table_t*, bool, const char*, int, const void* );
        return vfunc< fn > ( this, 8 )( this, is_server, value, length, userdata );
    }
};

class c_network_string_table_container {
public:
    network_string_table_t* find_table ( const char* table_name ) {
        using fn = network_string_table_t * ( __thiscall* )( c_network_string_table_container*, const char* );
        return vfunc< fn > ( this, 3 )( this, table_name );
    }
};