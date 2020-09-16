#pragma once
#include <cstdint>
#include <string_view>
#include "../utils/vfunc.hpp"

enum cvar_flags_t : uint32_t {
	fcvar_none = 0,
	fcvar_unregistered = ( 1 << 0 ),
	fcvar_developmentonly = ( 1 << 1 ),
	fcvar_gamedll = ( 1 << 2 ),
	fcvar_clientdll = ( 1 << 3 ),
	fcvar_hidden = ( 1 << 4 ),
	fcvar_protected = ( 1 << 5 ),
	fcvar_sponly = ( 1 << 6 ),
	fcvar_archive = ( 1 << 7 ),
	fcvar_notify = ( 1 << 8 ),
	fcvar_userinfo = ( 1 << 9 ),
	fcvar_cheat = ( 1 << 14 ),
	fcvar_printableonly = ( 1 << 10 ),
	fcvar_unlogged = ( 1 << 11 ),
	fcvar_never_as_string = ( 1 << 12 ),
	fcvar_replicated = ( 1 << 13 ),
	fcvar_demo = ( 1 << 16 ),
	fcvar_dontrecord = ( 1 << 17 ),
	fcvar_not_connected = ( 1 << 22 ),
	fcvar_archive_xbox = ( 1 << 24 ),
	fcvar_server_can_execute = ( 1 << 28 ),
	fcvar_server_cannot_query = ( 1 << 29 ),
	fcvar_clientcmd_can_execute = ( 1 << 30 )
};

struct cvar_value_t {
	char* str;
	int str_len;
	float f;
	int i;
};

class cvar_t {
private:
	void* vtable;

public:
	cvar_t* m_next;
	int m_registered;
	char* m_name;
	char* m_help_string;
	int m_flags;
	void* m_callback;
	cvar_t* m_parent;
	char* m_default_value;
	cvar_value_t m_value;
	int m_has_min;
	float m_min;
	int m_has_max;
	float m_max;
	void* m_callbacks;

public:
	const char* get_string ( ) {
		if ( m_flags & cvar_flags_t::fcvar_never_as_string )
			return "FCVAR_NEVER_AS_STRING";

		return m_value.str ? m_value.str : "";
	}

	float get_float ( ) {
		uint32_t xored = *reinterpret_cast< uintptr_t* >( &m_value.f) ^ reinterpret_cast< uintptr_t >( this );
		return *reinterpret_cast< float* >(&xored);
	}

	int get_int ( ) {
		return static_cast<int> ( m_value.i ^ reinterpret_cast< uintptr_t >(this) );
	}

	bool get_bool ( ) {
		return !!get_int();
	}

	void set_value ( const char* value ) {
		return vfunc< void ( __thiscall* )( void*, const char* ) > ( this, 14 )( this, value );
	}

	void set_value ( float value ) {
		return vfunc< void ( __thiscall* )( void*, float ) > ( this, 15 )( this, value );
	}

	void set_value ( int value ) {
		return vfunc< void ( __thiscall* )( void*, int ) > ( this, 16 )( this, value );
	}

	void no_callback ( ) {
		*reinterpret_cast< int* >( reinterpret_cast< uintptr_t >( &m_callbacks ) + 0xc ) = 0;
	}
};

class c_cvar {
public:
	cvar_t* find( const char* var ) {
		return vfunc< cvar_t * ( __thiscall* )( void*, const char* ) > ( this, 14 )( this, var );
	}
};