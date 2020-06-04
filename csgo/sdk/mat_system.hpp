#pragma once
#include "../utils/vfunc.hpp"
#include "../utils/padding.hpp"

class material_t {
public:
	const char* get_name( void ) {
		using getname_fn = const char* ( __thiscall* )( void* );
		return vfunc< getname_fn >( this, 0 )( this );
	}

	const char* get_texture_group_name( void ) {
		using gettexturegroupname_fn = const char* ( __thiscall* )( void* );
		return vfunc< gettexturegroupname_fn >( this, 1 )( this );
	}

	void increment_reference_count( void ) {
		using increferencecount_fn = void( __thiscall* )( void* );
		vfunc< increferencecount_fn >( this, 12 )( this );
	}

	void* find_var( const char* var, bool* found, bool complain = true ) {
		using findvar_fn = void* ( __thiscall* )( void*, const char*, bool*, bool );
		return vfunc< findvar_fn >( this, 11 )( this, var, found, complain );
	}

	void color_modulate ( int r, int g, int b ) {
		using colormodulate_fn = void ( __thiscall* )( void*, float, float, float );
		vfunc< colormodulate_fn > ( this, 28 )( this, static_cast< float >( r ) / 255.0f, static_cast< float >( g ) / 255.0f, static_cast< float >( b ) / 255.0f );
	}

	void alpha_modulate ( int a ) {
		using colormodulate_fn = void ( __thiscall* )( void*, float );
		vfunc< colormodulate_fn > ( this, 27 )( this, static_cast< float >( a ) / 255.0f );
	}

	void set_material_var_flag ( std::uint32_t flag, bool state ) {
		using setmaterialvarflag_fn = void ( __thiscall* )( void*, std::uint32_t, bool );
		vfunc< setmaterialvarflag_fn > ( this, 29 )( this, flag, state );
	}

	bool is_error_material ( ) {
		using is_error_material_fn = bool ( __thiscall* )( void* );
		return vfunc< is_error_material_fn > ( this, 42 )( this );
	}
};

class c_matsys {
public:
	material_t* createmat( const char* name, void* kv ) {
		using creatematerial_fn = material_t * ( __thiscall* )( void*, const char*, void* );
		return vfunc< creatematerial_fn >( this, 83 )( this, name, kv );
	}

	material_t* findmat( char const* name, const char* group_name, bool complain = true, const char* complain_prefix = nullptr ) {
		using findmaterial_fn = material_t * ( __thiscall* )( void*, char const*, const char*, bool, const char* );
		return vfunc< findmaterial_fn >( this, 84 )( this, name, group_name, complain, complain_prefix );
	}

	material_t* get_material ( int handle ) {
		using get_material_fn = material_t * ( __thiscall* )( void*, int );
		return vfunc< get_material_fn > ( this, 89 )( this, handle );
	}

	int first_material ( ) {
		using first_material_fn = int ( __thiscall* )( void* );
		return vfunc< first_material_fn > ( this, 86 )( this );
	}

	int next_material ( int handle ) {
		using next_material_fn = int ( __thiscall* )( void*, int );
		return vfunc< next_material_fn > ( this, 87 )( this, handle );
	}

	int invalid_material ( ) {
		using invalid_material_fn = int ( __thiscall* )( void* );
		return vfunc< invalid_material_fn > ( this, 88 )( this );
	}
};