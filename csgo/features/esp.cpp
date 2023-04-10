#include "esp.hpp"
#include "../menu/menu.hpp"
#include "../globals.hpp"
#include "ragebot.hpp"
#include "../animations/resolver.hpp"
#include "other_visuals.hpp"
#include "autowall.hpp"
#include <locale>

#include "../renderer/render.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"

#undef min
#undef max

#pragma region SHARED_ESP
inline unsigned int funny_array [ ] = {
	-1 , -2 , -4 , -8 , -16 , -32 , -64 , -128 , -256 , -512 , -1024 , -2048 , -4096 , -8192 , -16384 , -32768 , -65536 , -131072 , -262144 , -524288 , -1048576 , -2097152 , -4194304 , -8388608 , -16777216 , -33554432 , -67108864 , -134217728 , -268435456 , -536870912 , -1073741824 , -2147483648 , 0 , -1 , -3 , -7 , -15 , -31 , -63 , -127 , -255 , -511 , -1023 , -2047 , -4095 , -8191 , -16383 , -32767 , -65535 , -131071 , -262143 , -524287 , -1048575 , -2097151 , -4194303 , -8388607 , -16777215 , -33554431 , -67108863 , -134217727 , -268435455 , -536870911 , -1073741823 , -2147483647 , 1 , 1 , -1 , -5 , -13 , -29 , -61 , -125 , -253 , -509 , -1021 , -2045 , -4093 , -8189 , -16381 , -32765 , -65533 , -131069 , -262141 , -524285 , -1048573 , -2097149 , -4194301 , -8388605 , -16777213 , -33554429 , -67108861 , -134217725 , -268435453 , -536870909 , -1073741821 , -2147483645 , 3 , 3 , 3 , -1 , -9 , -25 , -57 , -121 , -249 , -505 , -1017 , -2041 , -4089 , -8185 , -16377 , -32761 , -65529 , -131065 , -262137 , -524281 , -1048569 , -2097145 , -4194297 , -8388601 , -16777209 , -33554425 , -67108857 , -134217721 , -268435449 , -536870905 , -1073741817 , -2147483641 , 7 , 7 , 7 , 7 , -1 , -17 , -49 , -113 , -241 , -497 , -1009 , -2033 , -4081 , -8177 , -16369 , -32753 , -65521 , -131057 , -262129 , -524273 , -1048561 , -2097137 , -4194289 , -8388593 , -16777201 , -33554417 , -67108849 , -134217713 , -268435441 , -536870897 , -1073741809 , -2147483633 , 15 , 15 , 15 , 15 , 15 , -1 , -33 , -97 , -225 , -481 , -993 , -2017 , -4065 , -8161 , -16353 , -32737 , -65505 , -131041 , -262113 , -524257 , -1048545 , -2097121 , -4194273 , -8388577 , -16777185 , -33554401 , -67108833 , -134217697 , -268435425 , -536870881 , -1073741793 , -2147483617 , 31 , 31 , 31 , 31 , 31 , 31 , -1 , -65 , -193 , -449 , -961 , -1985 , -4033 , -8129 , -16321 , -32705 , -65473 , -131009 , -262081 , -524225 , -1048513 , -2097089 , -4194241 , -8388545 , -16777153 , -33554369
};

inline unsigned int funny_array_part2 [ ] = {
	0 , 1 , 3 , 7 , 15 , 31 , 63 , 127 , 255 , 511 , 1023 , 2047 , 4095 , 8191 , 16383 , 32767 , 65535 , 131071 , 262143 , 524287 , 1048575 , 2097151 , 4194303 , 8388607 , 16777215 , 33554431 , 67108863 , 134217727 , 268435455 , 536870911 , 1073741823 , 2147483647 , -1 , 1869180533 , 110 , 0 , -2147483648
};

struct premium_shared_esp {
	int* field_0;
	int field_4;
	int field_8;
	int field_C;
	bool field_10;
	bool field_11;
};

struct funny_t {
	int _1;
	int _2;
	int _3;
	int _4;
};

struct SomeSharedESPData {
	int gap0;
	bool byte4;
	char field_5;
	char field_6;
	char field_7;
	int field_8;
	int field_C;
	int dword10;
	int dword14;
	unsigned int* punsigned_int18;
	unsigned int* field_1C;
	funny_t punsigned_int1C;
	int aaaaaa;
	int aaaaaa_part2;
	unsigned int dlwsakd;
};

void set_voice_data ( premium_shared_esp* ecx, unsigned int a2, int a3 ) {
	int v5; // ecx
	int v6; // edx
	int v7; // ebp
	int v8; // ecx
	int v9; // ebp

	v5 = ecx->field_C;
	v6 = ecx->field_8;
	if ( v5 + a3 <= v6 ) {
		v7 = v5;
		v8 = v5 & 0x1F;
		v9 = v7 >> 5;
		ecx->field_0 [ v9 ] = ( a2 << v8 ) | ecx->field_0 [ v9 ] & funny_array [ 0x21 * v8 + a3 ];
		if ( 32 - v8 < a3 )
			ecx->field_0 [ v9 + 1 ] = ( a2 >> ( 32 - v8 ) ) | ecx->field_0 [ v9 + 1 ] & funny_array [ a3 - ( 32 - v8 ) ];
		ecx->field_C += a3;
	}
	else {
		ecx->field_C = v6;
		ecx->field_10 = 1;
	}
}

int set_voice_data_2 ( premium_shared_esp* _ecx, int a2 ) {
	int v2; // edx
	int result; // eax
	char v4; // dl
	char* v5; // esi
	int v6; // eax

	v2 = _ecx->field_C;
	result = v2 + 1;
	if ( v2 + 1 <= _ecx->field_8 ) {
		if ( !_ecx->field_10 ) {
			v4 = v2 & 7;
			v5 = ( char* ) ( ( int ) _ecx->field_0 + ( _ecx->field_C >> 3 ) );
			v6 = *v5;
			if ( a2 )
				result = v6 | ( 1 << v4 );
			else
				result = v6 & ~( 1 << v4 );
			*v5 = result;
			++_ecx->field_C;
		}
	}
	else {
		_ecx->field_10 = 1;
	}
	return result;
}

void setup_coord ( premium_shared_esp* a2, float a1 ) {
	int v3; // ebx
	unsigned int v4; // esi

	v3 = ( int ) abs ( a1 );
	v4 = abs ( ( int ) ( a1 * 32.0 ) ) & 31;
	set_voice_data_2 ( a2, v3 );
	set_voice_data_2 ( a2, v4 );
	if ( v3 || v4 ) {
		set_voice_data_2 ( a2, a1 <= -0.03125 );
		if ( v3 )
			set_voice_data ( a2, v3 - 1, 14 );
		if ( v4 )
			set_voice_data ( a2, v4, 5 );
	}
}

void setup_esp_data ( SomeSharedESPData* _ecx, int a2 ) {
	int v3; // ebx
	int v4; // edi
	unsigned int v5; // ebp
	int v6; // ecx
	signed int v7; // edi
	unsigned int* v8; // eax
	unsigned int v9; // edx
	int v10; // ecx
	int v11; // edx
	int v12; // eax
	int v13; // eax
	int v14; // eax

	v3 = 1;
	v4 = 0;
	if ( _ecx->field_8 < 0 ) {
		v4 = _ecx->field_8;
		_ecx->byte4 = 1;
	}
	v5 = _ecx->field_C & 3;
	if ( _ecx->field_C >= 4u && ( !v5 || v4 / 8 >= ( int ) v5 ) ) {
		v6 = _ecx->punsigned_int1C._1;
		v7 = v4 - 8 * v5;
		v8 = ( unsigned int* ) ( v5 + v6 + 4 * ( v7 / 32 ) );
		_ecx->punsigned_int18 = v8;
		if ( v6 ) {
			_ecx->dword14 = 32;
			if ( v8 != _ecx->field_1C ) {
				if ( v8 <= _ecx->field_1C ) {
					v9 = *v8;
					v3 = 32;
					_ecx->punsigned_int18 = v8 + 1;
				LABEL_11:
					_ecx->dword10 = v9 >> ( v7 & 0x1F );
					if ( v3 >= 32 - ( v7 & 0x1F ) )
						v3 = 32 - ( v7 & 0x1F );
					goto LABEL_23;
				}
				_ecx->byte4 = 1;
				v3 = 32;
			LABEL_10:
				v9 = 0;
				goto LABEL_11;
			}
			_ecx->punsigned_int18 = v8 + 1;
		}
		_ecx->dword14 = 1;
		goto LABEL_10;
	}
	v10 = _ecx->punsigned_int1C._1;
	if ( v10 ) {
		v11 = *( unsigned __int8* ) v10++;
		_ecx->dword10 = v11;
		if ( v5 > 1 ) {
			v12 = *( unsigned __int8* ) v10++;
			v13 = v11 | ( v12 << 8 );
			_ecx->dword10 = v13;
			v11 = v13;
		}
		if ( v5 > 2 ) {
			v14 = *( unsigned __int8* ) v10++;
			_ecx->dword10 = v11 | ( v14 << 16 );
		}
	}
	_ecx->punsigned_int18 = ( unsigned int* ) v10;
	_ecx->dword10 >>= v4 & 0x1F;
	v3 = 8 * v5 - ( v4 & 0x1F );
LABEL_23:
	_ecx->dword14 = v3;
}

/* gamesense.idb - SharedESP::CreateMove */
void features::esp::send_data_to_clients ( ) {
	VMP_BEGINMUTATION ( );
	static auto c_clc_msg_voice_data__constructor = pattern::search ( _ ( "engine.dll" ), _ ( "56 57 8B F9 8D 4F 08 C7 07 ? ? ? ? E8 ? ? ? ? C7 07 ? ? ? ?" ) ).get< void ( __thiscall* )( void* ) > ( );
	static auto c_clc_msg_voice_data__deconstructor = pattern::search ( _ ( "engine.dll" ), _ ( "E8 ? ? ? ? 5E 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC 51" ) ).resolve_rip().get< void ( __thiscall* )( void* ) > ( );

	constexpr bool SHARE_WITH_OTHER_TEAM = true;

	static auto sv_deadtalk = cs::i::cvar->find ( _ ( "sv_deadtalk" ) );
	static auto sv_full_alltalk = cs::i::cvar->find ( _ ( "sv_full_alltalk" ) );

	static int funny_counter = 1;
	static float last_shared_time = 0.0f;

	if ( abs ( cs::i::globals->m_curtime - last_shared_time ) < 0.0625f ||
		!cs::i::client_state->net_channel ( ) ||
		!sv_deadtalk->get_bool ( ) &&
		!sv_full_alltalk->get_bool ( ) )
		return;

	for ( int i = 0; i <= cs::i::globals->m_max_clients; i++ ) {
		auto player = cs::i::ent_list->get< player_t* > ( i );

		if ( !player || !player->is_player() )
			continue;

		if ( *reinterpret_cast< bool* >( reinterpret_cast< int >( player ) + 0x3624 ) || player->dormant ( ) || !player->alive ( ) )
			continue;

		int unk = vfunc< int ( __thiscall* )( void* ) > ( player, 121 )( player );

		if ( unk <= 0 )
			continue;

		int esp_id = ( i + funny_counter ) % cs::i::globals->m_max_clients;
		int array_data [ 5 ];

		premium_shared_esp unknown_esp_data { };
		unknown_esp_data.field_10 = false;
		unknown_esp_data.field_11 = true;
		unknown_esp_data.field_4 = 20;
		unknown_esp_data.field_C = 0;
		unknown_esp_data.field_8 = 160;
		unknown_esp_data.field_0 = array_data;

		int v17 = 127;

		if ( unk <= 127 ) {
			v17 = unk;
			if ( unk < 0 )
				v17 = 0;
		}

		auto origin = player->origin ( );

		set_voice_data ( &unknown_esp_data, 0xBEEFu, 0x10 );
		set_voice_data ( &unknown_esp_data, esp_id, 7 );

		setup_coord ( &unknown_esp_data, origin.x );
		setup_coord ( &unknown_esp_data, origin.y );
		setup_coord ( &unknown_esp_data, origin.z );

		set_voice_data ( &unknown_esp_data, g::server_tick, 32 );
		set_voice_data ( &unknown_esp_data, v17, 7 );
		set_voice_data ( &unknown_esp_data, 0, 32 );

		if ( !unknown_esp_data.field_10 ) {
			uint8_t voice_buffer [ 128 ];
			memset ( voice_buffer, 0, sizeof ( voice_buffer ) );
			c_clc_msg_voice_data__constructor ( voice_buffer );

			auto voice_msg = ( voice_data* ) voice_buffer;

			voice_msg->dword20 = 0;
			voice_msg->caster_flags |= 0x3Eu;
			voice_msg->voice_data = ( struct_dword18* ) array_data [ 3 ];
			voice_msg->gap1C = ( int ) array_data [ 4 ];
			voice_msg->dword28 = array_data [ 0 ];
			voice_msg->dword24 = array_data [ 1 ];
			voice_msg->dword2C = array_data [ 2 ];

			vfunc< bool ( __thiscall* ) ( net_channel_t*, void*, bool, bool )> ( cs::i::client_state->net_channel ( ), 40 )( cs::i::client_state->net_channel ( ), voice_msg, false, false );

			c_clc_msg_voice_data__deconstructor ( voice_msg );
		}

		funny_counter = esp_id + 1;
		last_shared_time = cs::i::globals->m_curtime;
		return;
	}
	VMP_END ( );
}

bool Main ( features::esp::voice_data* msg );

bool features::esp::recieve_voice_data ( voice_data* msg ) {
	return Main ( msg );
}

using _DWORD = int;

#undef LOBYTE

#define SLOBYTE(x)   (*((__int8*)&(x)))
#define LODWORD(x)  (*((int*)&(x)))  // low dword
#define LOBYTE(x)   (*((char*)&(x)))   // low byte

float sub_6ACAF3F1 ( SomeSharedESPData* _ecx ) {
	char v2; // bl
	float result; // xmm0_4
	int v4; // edi
	int v5; // eax
	unsigned int* v6; // eax
	unsigned int v7; // esi
	int v8; // ecx
	int v9; // eax
	unsigned int* v10; // eax
	unsigned int v11; // esi
	int v12; // eax
	unsigned int* v13; // eax
	unsigned int v14; // esi
	unsigned int* v15; // eax
	unsigned int* v16; // eax
	unsigned int* v17; // eax
	unsigned int v18; // eax
	unsigned int v19; // esi
	unsigned int* v20; // eax
	unsigned int* v21; // eax
	unsigned int* v22; // eax
	int v23; // ecx
	unsigned int v24; // eax
	int v25; // [esp+8h] [ebp-18h]
	int v26; // [esp+10h] [ebp-10h]
	int v27; // [esp+10h] [ebp-10h]
	int v28; // [esp+14h] [ebp-Ch]
	unsigned int v29; // [esp+14h] [ebp-Ch]
	char v30; // [esp+18h] [ebp-8h]
	char v31; // [esp+1Fh] [ebp-1h]

	v2 = 1;
	result = 0.0;
	v26 = _ecx->dword10 & 1;
	v4 = 32;
	v5 = _ecx->dword14 - 1;
	if ( _ecx->dword14 == 1 ) {
		v6 = _ecx->punsigned_int18;
		_ecx->dword14 = 32;
		if ( v6 == _ecx->field_1C ) {
			v7 = 0;
			_ecx->punsigned_int18 = v6 + 1;
			v5 = 1;
		}
		else {
			if ( v6 <= _ecx->field_1C ) {
				v7 = *v6;
				_ecx->punsigned_int18 = v6 + 1;
			}
			else {
				_ecx->byte4 = 1;
				v7 = 0;
			}
			v5 = 32;
		}
	}
	else {
		v7 = _ecx->dword10 >> 1;
	}
	_ecx->dword10 = v7;
	v8 = v7 & 1;
	v9 = v5 - 1;
	_ecx->dword14 = v9;
	if ( v9 ) {
		v11 = v7 >> 1;
	}
	else {
		v10 = _ecx->punsigned_int18;
		_ecx->dword14 = 32;
		if ( v10 == _ecx->field_1C ) {
			_ecx->dword14 = 1;
			_ecx->punsigned_int18 = v10 + 1;
			v11 = 0;
			v9 = 1;
		}
		else {
			if ( v10 <= _ecx->field_1C ) {
				v11 = *v10;
				_ecx->punsigned_int18 = v10 + 1;
			}
			else {
				_ecx->byte4 = 1;
				v11 = 0;
			}
			v9 = 32;
		}
	}
	_ecx->dword10 = v11;
	if ( v26 || v8 ) {
		v25 = v11 & 1;
		v12 = v9 - 1;
		v30 = v12;
		_ecx->dword14 = v12;
		if ( v12 ) {
			v14 = v11 >> 1;
			v4 = v12;
		}
		else {
			v13 = _ecx->punsigned_int18;
			_ecx->dword14 = 32;
			if ( v13 == _ecx->field_1C ) {
				_ecx->dword14 = 1;
				_ecx->punsigned_int18 = v13 + 1;
				v14 = 0;
				v12 = 1;
				v4 = 1;
				v30 = 1;
			}
			else if ( v13 <= _ecx->field_1C ) {
				v14 = *v13;
				_ecx->punsigned_int18 = v13 + 1;
				v12 = 32;
				v30 = 32;
			}
			else {
				v12 = 32;
				_ecx->byte4 = 1;
				v30 = 32;
				v14 = 0;
			}
		}
		_ecx->dword10 = v14;
		if ( !v26 ) {
		LABEL_47:
			if ( !v8 )
				goto LABEL_66;
			if ( v4 < 5 ) {
				v22 = _ecx->punsigned_int18;
				v23 = 5 - v4;
				if ( v22 == _ecx->field_1C ) {
					_ecx->dword14 = 1;
					_ecx->punsigned_int18 = v22 + 1;
					LOBYTE ( v4 ) = 1;
					_ecx->byte4 = 1;
					v24 = 0;
				}
				else if ( v22 <= _ecx->field_1C ) {
					v24 = *v22;
					++_ecx->punsigned_int18;
					v2 = _ecx->byte4;
				}
				else {
					_ecx->byte4 = 1;
					v24 = 0;
				}
				_ecx->dword10 = v24;
				if ( v2 ) {
					v8 = 0;
				}
				else {
					_ecx->dword14 = 32 - v23;
					_ecx->dword10 = v24 >> v23;
					v8 = v14 | ( ( v24 & funny_array_part2 [ v23 ] ) << v4 );
				}
				goto LABEL_66;
			}
			v8 = v14 & 0x1F;
			_ecx->dword14 = v4 - 5;
			if ( v4 != 5 ) {
				v19 = v14 >> 5;
			LABEL_57:
				_ecx->dword10 = v19;
			LABEL_66:
				result = ( float ) v8 * 0.03125 + ( double ) v26;
				if ( v25 )
					result = -result;
				return result;
			}
			v20 = _ecx->punsigned_int18;
			_ecx->dword14 = 32;
			if ( v20 == _ecx->field_1C ) {
				v21 = v20 + 1;
				_ecx->dword14 = 1;
				v19 = 0;
			}
			else {
				if ( v20 > _ecx->field_1C ) {
					_ecx->byte4 = 1;
					v19 = 0;
					goto LABEL_57;
				}
				v19 = *v20;
				v21 = v20 + 1;
			}
			_ecx->punsigned_int18 = v21;
			goto LABEL_57;
		}
		if ( v12 < 14 ) {
			v27 = 14 - v12;
			v17 = _ecx->punsigned_int18;
			if ( v17 == _ecx->field_1C ) {
				_ecx->dword14 = 1;
				_ecx->punsigned_int18 = v17 + 1;
				v4 = 1;
				v18 = 0;
				_ecx->byte4 = 1;
				v29 = 0;
				v30 = 1;
				v31 = 1;
			}
			else if ( v17 <= _ecx->field_1C ) {
				v18 = *v17;
				++_ecx->punsigned_int18;
				v29 = v18;
				v31 = _ecx->byte4;
			}
			else {
				v18 = 0;
				_ecx->byte4 = 1;
				v29 = 0;
				v31 = 1;
			}
			_ecx->dword10 = v18;
			if ( v31 ) {
				v28 = 0;
			}
			else {
				v18 >>= v27;
				v4 = 32 - v27;
				_ecx->dword10 = v18;
				v28 = v14 | ( ( funny_array_part2 [ v27 ] & v29 ) << v30 );
				_ecx->dword14 = 32 - v27;
			}
			v14 = v18;
			goto LABEL_46;
		}
		v28 = v14 & 0x3FFF;
		v4 = v12 - 14;
		_ecx->dword14 = v12 - 14;
		if ( v12 != 14 ) {
			v14 >>= 14;
		LABEL_36:
			_ecx->dword10 = v14;
		LABEL_46:
			v26 = v28 + 1;
			goto LABEL_47;
		}
		v15 = _ecx->punsigned_int18;
		v4 = 32;
		_ecx->dword14 = 32;
		if ( v15 == _ecx->field_1C ) {
			v16 = v15 + 1;
			_ecx->dword14 = 1;
			v14 = 0;
			v4 = 1;
		}
		else {
			if ( v15 > _ecx->field_1C ) {
				_ecx->byte4 = 1;
				v14 = 0;
				goto LABEL_36;
			}
			v14 = *v15;
			v16 = v15 + 1;
		}
		_ecx->punsigned_int18 = v16;
		goto LABEL_36;
	}
	return result;
}

bool Main ( features::esp::voice_data* msg ) {
	int flags; // ecx
	int v8; // edi
	unsigned int* v9; // edx
	unsigned int v10; // esi
	unsigned __int16 v11; // ax
	unsigned int* v12; // ecx
	int v13; // eax
	unsigned int v14; // eax
	unsigned int v15; // esi
	unsigned int* v16; // edx
	unsigned int v17; // ecx
	float v18; // xmm0_4
	unsigned int v19; // xmm1_4
	int v20; // ecx
	unsigned int* v21; // edi
	unsigned int v22; // edx
	unsigned int* v23; // eax
	unsigned int v24; // esi
	int v25; // edx
	int v26; // eax
	int* v27; // esi
	float* v28; // edi
	SomeSharedESPData v30; // [esp-64h] [ebp-70h] BYREF
	float v31; // [esp-24h] [ebp-30h]
	int v32; // [esp-20h] [ebp-2Ch]
	unsigned int v33; // [esp-1Ch] [ebp-28h]
	unsigned int v34; // [esp-18h] [ebp-24h]
	float v35; // [esp-14h] [ebp-20h]
	int v36; // [esp-10h] [ebp-1Ch]
	int v37; // [esp-Ch] [ebp-18h]
	unsigned int v38; // [esp-8h] [ebp-14h]
	unsigned int* v39; // [esp-4h] [ebp-10h]
	int v40; // [esp+0h] [ebp-Ch]
	int v41; // [esp+4h] [ebp-8h]

	static auto sv_deadtalk = cs::i::cvar->find ( _ ( "sv_deadtalk" ) );
	static auto sv_full_alltalk = cs::i::cvar->find ( _ ( "sv_full_alltalk" ) );

	v35 = *( float* ) &msg;
	if ( !sv_deadtalk->get_bool ( ) && !sv_full_alltalk->get_bool ( ) ) {
		return 0;
	}
	flags = msg->caster_flags;
	if ( ( flags & 0x10 ) != 0 ) {
		if ( msg->voice_data->size )
			return 0;
	}
	if ( ( flags & 0x40 ) == 0 )
		return 0;
	if ( msg->dword20 )
		return 0;
	if ( ( flags & 0x185 ) != 0x185 )
		return 0;
	v30.punsigned_int1C._2 = msg->dword28;
	v30.punsigned_int1C._3 = msg->dword24;
	v30.punsigned_int1C._4 = msg->dword2C;
	v30.aaaaaa = *( _DWORD* ) &msg->gapC [ 4 ];
	v30.aaaaaa_part2 = *( _DWORD* ) &msg->gapC [ 8 ];
	v30.punsigned_int1C._1 = ( int ) &v30.punsigned_int1C._2;
	v30.punsigned_int18 = ( unsigned int* ) &v30.punsigned_int1C._2;
	v30.gap0 = 0;
	v30.field_C = 0x14;
	v30.field_8 = 0xA0;
	v30.byte4 = 0;
	v30.field_1C = &v30.dlwsakd;
	setup_esp_data ( &v30, 0x185 );
	v8 = v30.dword14;
	v9 = v30.punsigned_int18;
	if ( ( int ) v30.dword14 >= 16 ) {
		v8 = v30.dword14 - 16;
		v30.dword14 = v8;
		if ( v8 ) {
			v10 = HIWORD ( v30.dword10 );
		LABEL_26:
			v11 = v30.dword10;
		LABEL_34:
			v12 = v30.field_1C;
			goto LABEL_35;
		}
		v8 = 32;
		v30.dword14 = 32;
		if ( v30.punsigned_int18 == v30.field_1C ) {
			v9 = v30.punsigned_int18 + 1;
			v10 = 0;
			v8 = 1;
			v30.dword14 = 1;
		}
		else {
			if ( v30.punsigned_int18 > v30.field_1C ) {
				v10 = 0;
				v30.byte4 = 1;
				goto LABEL_26;
			}
			v10 = *v30.punsigned_int18;
			v9 = v30.punsigned_int18 + 1;
		}
		v30.punsigned_int18 = v9;
		goto LABEL_26;
	}
	v12 = v30.field_1C;
	v33 = 16 - v30.dword14;
	if ( v30.punsigned_int18 == v30.field_1C ) {
		v10 = 0;
		v9 = v30.punsigned_int18 + 1;
		v8 = 1;
		++v30.punsigned_int18;
		v30.dword14 = 1;
		v30.byte4 = 1;
	}
	else if ( v30.punsigned_int18 <= v30.field_1C ) {
		v10 = *v30.punsigned_int18;
		v9 = ++v30.punsigned_int18;
		if ( !v30.byte4 ) {
			v34 = ( ( v10 & funny_array_part2 [ 16 - v30.dword14 ] ) << SLOBYTE ( v30.dword14 ) ) | v30.dword10;
			v11 = v34;
			v8 = 32 - v33;
			v10 >>= v33;
			v30.dword14 = 32 - v33;
			goto LABEL_34;
		}
	}
	else {
		v10 = 0;
		v30.byte4 = 1;
	}
	v11 = 0;
LABEL_35:
	v34 = v11;
	v13 = v11 - 0xBEED;
	if ( !v13 || v13 == 2 ) {
		v33 = *( _DWORD* ) ( LODWORD ( v35 ) + 8 );
		if ( ( WORD ) v34 != 0xBEEF )
			return 1;
		if ( v8 < 7 ) {
			v33 = 7 - v8;
			if ( v9 == v12 ) {
				v30.dword10 = 0;
				v30.dword14 = 1;
				v30.punsigned_int18 = v9 + 1;
				v30.byte4 = 1;
			}
			else if ( v9 <= v12 ) {
				v17 = *v9;
				v34 = v17;
				v30.dword10 = v17;
				v30.punsigned_int18 = v9 + 1;
				if ( !v30.byte4 ) {
					LODWORD ( v35 ) = ( ( v17 & funny_array_part2 [ 7 - v8 ] ) << v8 ) | v10;
					v30.dword14 = 32 - v33;
					v30.dword10 = v34 >> v33;
					v14 = LODWORD ( v35 );
					goto LABEL_59;
				}
			}
			else {
				v30.dword10 = 0;
				v30.byte4 = 1;
			}
			v14 = 0;
		LABEL_59:
			v33 = v14 + 1;
			if ( v14 > 0x3F )
				return 1;
			v35 = sub_6ACAF3F1 ( &v30 );
			*( float* ) &v34 = sub_6ACAF3F1 ( &v30 );
			*( float* ) &v30.dlwsakd = v35;
			*( &v30.dlwsakd + 1 ) = v34;
			v18 = sub_6ACAF3F1 ( &v30 );
			v31 = v18;
			v19 = v34;
			*( float* ) &v34 = v35;
			v20 = 0x7F800000;
			if ( ( LODWORD ( v35 ) & 0x7F800000 ) == 0x7F800000 )
				return 1;
			if ( v35 <= -16384.0 )
				return 1;
			if ( v35 >= 16384.0 )
				return 1;
			v34 = v19;
			if ( ( v19 & 0x7F800000 ) == 0x7F800000 )
				return 1;
			if ( *( float* ) &v19 <= -16384.0 )
				return 1;
			if ( *( float* ) &v19 >= 16384.0 )
				return 1;
			*( float* ) &v34 = v18;
			if ( ( LODWORD ( v18 ) & 0x7F800000 ) == 0x7F800000 || v18 <= -16384.0 || v18 >= 16384.0 )
				return 1;
			v21 = v30.punsigned_int18;
			v37 = v30.dword14;
			if ( ( int ) v30.dword14 >= 32 ) {
				v35 = *( float* ) &v30.dword10;
				v37 = v30.dword14 - 32;
				if ( v30.dword14 == 32 ) {
					v37 = 32;
					v23 = v30.field_1C;
					if ( v30.punsigned_int18 != v30.field_1C ) {
						if ( v30.punsigned_int18 <= v30.field_1C ) {
							LOBYTE ( v23 ) = v30.byte4;
							v22 = *v30.punsigned_int18;
							v21 = v30.punsigned_int18 + 1;
							v39 = v23;
						}
						else {
							v22 = 0;
							LOBYTE ( v39 ) = 1;
						}
						goto LABEL_77;
					}
					v21 = v30.punsigned_int18 + 1;
					v37 = 1;
				}
				LOBYTE ( v20 ) = v30.byte4;
				v22 = 0;
				v39 = ( unsigned int* ) v20;
			LABEL_77:
				v38 = v22;
				v36 = v22;
				goto LABEL_86;
			}
			v34 = 32 - v30.dword14;
			if ( v30.punsigned_int18 == v30.field_1C ) {
				v21 = v30.punsigned_int18 + 1;
				LOBYTE ( v39 ) = 1;
				v37 = 1;
			}
			else {
				if ( v30.punsigned_int18 <= v30.field_1C ) {
					v24 = *v30.punsigned_int18;
					v21 = v30.punsigned_int18 + 1;
					LOBYTE ( v20 ) = v30.byte4;
					v38 = v24;
					v36 = v24;
					v39 = ( unsigned int* ) v20;
					if ( !v30.byte4 ) {
						LODWORD ( v35 ) = ( ( v24 & funny_array_part2 [ v34 ] ) << SLOBYTE ( v30.dword14 ) ) | v30.dword10;
						v37 = 32 - v34;
						v38 = v24 >> v34;
						v36 = v24 >> v34;
					LABEL_86:
						if ( ( int ) abs ( LODWORD ( v35 ) - (int)g::server_tick ) <= ( int ) ( float ) ( ( float ) ( 1.0 / cs::i::globals->m_ipt ) + 0.5 ) ) {
							if ( v37 >= 7 ) {
								v25 = v36 & 0x7F;
								if ( v37 == 7 && v21 != v30.field_1C ) {
									v26 = ( unsigned __int8 ) v39;
									if ( v21 > v30.field_1C )
										v26 = 1;
									v39 = ( unsigned int* ) v26;
								}
								goto LABEL_99;
							}
							if ( v21 == v30.field_1C || v21 > v30.field_1C ) {
								LOBYTE ( v39 ) = 1;
							}
							else if ( !( bool ) v39 ) {
								v25 = v38 | ( ( *v21 & funny_array_part2 [ 7 - v37 ] ) << v37 );
							LABEL_99:
								if ( ( unsigned int ) ( v25 - 1 ) <= 124 && !( bool ) v39 ) {
									auto player = cs::i::ent_list->get< player_t* > ( v33 );

									if ( player && player->is_player ( ) ) {
										if ( g::local && player->dormant ( ) && !player->life_state ( ) && g::local->is_enemy ( player ) ) {
											features::esp::esp_data [ player->idx ( ) ].m_sound_pos = vec3_t ( *( float* ) &v30.dlwsakd, *( float* ) &v19, v31 );
											features::esp::esp_data [ player->idx ( ) ].m_dormant = true;
											features::esp::esp_data [ player->idx ( ) ].m_last_seen = cs::i::globals->m_curtime;
										}
									}
								}
								return 1;
							}
							v25 = 0;
							goto LABEL_99;
						}
						return 1;
					}
				LABEL_84:
					v35 = 0.0;
					goto LABEL_86;
				}
				LOBYTE ( v39 ) = 1;
			}
			v38 = 0;
			v36 = 0;
			goto LABEL_84;
		}
		v14 = v10 & 0x7F;
		v30.dword14 = v8 - 7;
		if ( v8 != 7 ) {
			v15 = v10 >> 7;
		LABEL_51:
			v30.dword10 = v15;
			goto LABEL_59;
		}
		v30.dword14 = 32;
		if ( v9 == v12 ) {
			v16 = v9 + 1;
			v30.dword14 = 1;
			v15 = 0;
		}
		else {
			if ( v9 > v12 ) {
				v30.byte4 = 1;
				v15 = 0;
				goto LABEL_51;
			}
			v15 = *v9;
			v16 = v9 + 1;
		}
		v30.punsigned_int18 = v16;
		goto LABEL_51;
	}
	return 0;
}
#pragma endregion

float box_alpha = 0.0f;

void draw_esp_box( int x , int y , int w , int h , bool dormant , const options::option::colorf& esp_box_color ) {
	render::outline( x - 1 , y - 1 , w + 2 , h + 2 , rgba( 0 , 0 , 0 , std::clamp< int >( esp_box_color.a * 60.0f * box_alpha , 0 , 60 ) ) );
	render::outline( x , y , w , h , dormant ? rgba( 150 , 150 , 150 , static_cast< int > ( esp_box_color.a * 255.0f * box_alpha ) ) : rgba( static_cast< int > ( esp_box_color.r * 255.0f ) , static_cast< int > ( esp_box_color.g * 255.0f ) , static_cast< int > ( esp_box_color.b * 255.0f ) , static_cast< int >( esp_box_color.a * 255.0f * box_alpha ) ) );
}

auto cur_offset_left_height = 0;
auto cur_offset_right_height = 0;
auto cur_offset_left = 4;
auto cur_offset_right = 4;
auto cur_offset_bottom = 4;
auto cur_offset_top = 4;

enum esp_type_t {
	esp_type_bar = 0 ,
	esp_type_text ,
	esp_type_number ,
	esp_type_flag
};

void draw_esp_widget( const ImRect& box , const options::option::colorf& widget_color , esp_type_t type , bool show_value , const int orientation , bool dormant , double value , double max , std::string to_print = _( "" ) ) {
	int esp_bar_thickness = 2;

	uint32_t clr1 = rgba( 0 , 0 , 0 , std::clamp< int >( static_cast< float >( widget_color.a * 255.0f ) / 2.0f , 0 , 125 ) );
	uint32_t clr = rgba( static_cast< int > ( widget_color.r * 255.0f ) , static_cast< int > ( widget_color.g * 255.0f ) , static_cast< int > ( widget_color.b * 255.0f ) , static_cast< int > ( widget_color.a * 255.0f ) );

	if ( dormant ) {
		clr1 = rgba( 0 , 0 , 0 , std::clamp< int >( static_cast< float >( widget_color.a * 255.0f ) / 2.0f * box_alpha , 0 , 125 ) );
		clr = rgba( 150 , 150 , 150 , static_cast< int >( widget_color.a * 255.0f * box_alpha ) );
	}

	switch ( type ) {
	case esp_type_t::esp_type_bar: {
		const auto sval = std::to_string( static_cast< int >( value ) );

		vec3_t text_size;
		render::text_size( sval , _( "esp_font" ) , text_size );

		const auto fraction = std::clamp( value / max , 0.0 , 1.0 );
		const auto calc_height = fraction * box.Max.y;

		switch ( orientation ) {
		case features::esp_placement_t::esp_placement_left:
			render::rect( box.Min.x - cur_offset_left - esp_bar_thickness , box.Min.y + ( box.Max.y - calc_height ) , esp_bar_thickness , calc_height , clr );
			render::outline( box.Min.x - cur_offset_left - esp_bar_thickness - 1, box.Min.y - 1, esp_bar_thickness + 2, box.Max.y + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x - cur_offset_left - esp_bar_thickness + 1 + esp_bar_thickness / 2 - text_size.x / 2 , box.Min.y + ( box.Max.y - calc_height ) + 1 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast<int>( 255.0f * box_alpha ) ) , true );
			cur_offset_left += 7;
			break;
		case features::esp_placement_t::esp_placement_right:
			render::rect( box.Min.x + box.Max.x + cur_offset_right , box.Min.y + ( box.Max.y - calc_height ) , esp_bar_thickness , calc_height , clr );
			render::outline( box.Min.x + box.Max.x + cur_offset_right - 1, box.Min.y - 1, esp_bar_thickness + 2, box.Max.y + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x + box.Max.x + cur_offset_right + 1 + esp_bar_thickness / 2 - text_size.x / 2 , box.Min.y + ( box.Max.y - calc_height ) + 1 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_right += 7;
			break;
		case features::esp_placement_t::esp_placement_bottom:
			render::rect( box.Min.x + 1 , box.Min.y + box.Max.y + cur_offset_bottom , static_cast< float >( box.Max.x ) * fraction , esp_bar_thickness , clr );
			render::outline( box.Min.x - 1, box.Min.y + box.Max.y + cur_offset_bottom - 1, box.Max.x + 2, esp_bar_thickness + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x + 1 + static_cast< float >( box.Max.x ) * fraction + 1 - text_size.x / 2 , box.Min.y + box.Max.y + cur_offset_bottom + 1 + esp_bar_thickness / 2 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_bottom += 7;
			break;
		case features::esp_placement_t::esp_placement_top:
			render::rect( box.Min.x + 1 , box.Min.y - cur_offset_top - esp_bar_thickness , static_cast< float >( box.Max.x ) * fraction , esp_bar_thickness , clr );
			render::outline( box.Min.x - 1, box.Min.y - cur_offset_top - esp_bar_thickness - 1, box.Max.x + 2, esp_bar_thickness + 2, clr1 );

			if ( show_value )
				render::text( box.Min.x + 1 + static_cast< float >( box.Max.x ) * fraction + 1 - text_size.x / 2 , box.Min.y - cur_offset_top - esp_bar_thickness + 1 + esp_bar_thickness / 2 - text_size.y / 2 , sval , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_top += 7;
			break;
		}
	} break;
	case esp_type_t::esp_type_text: {
		std::string as_str = to_print;

		vec3_t text_size;
		render::text_size( as_str , _( "esp_font" ) , text_size );

		switch ( orientation ) {
		case features::esp_placement_t::esp_placement_left:
			render::gradient( box.Min.x - cur_offset_left - text_size.x - 2.0f , box.Min.y + cur_offset_left_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x - cur_offset_left - text_size.x - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y + cur_offset_left_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x - cur_offset_left - text_size.x - 2.0f , box.Min.y + cur_offset_left_height - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x - cur_offset_left - text_size.x , box.Min.y + cur_offset_left_height , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_left_height += text_size.y + 6;
			break;
		case features::esp_placement_t::esp_placement_right:
			render::gradient( box.Min.x + cur_offset_right + box.Max.x - 2.0f , box.Min.y + cur_offset_right_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x + cur_offset_right + box.Max.x - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y + cur_offset_right_height - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x + cur_offset_right + box.Max.x - 2.0f , box.Min.y + cur_offset_right_height - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x + cur_offset_right + box.Max.x , box.Min.y + cur_offset_right_height , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_right_height += text_size.y + 6;
			break;
		case features::esp_placement_t::esp_placement_bottom:
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y + box.Max.y + cur_offset_bottom - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y + box.Max.y + cur_offset_bottom - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y + box.Max.y + cur_offset_bottom - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x + box.Max.x / 2 - text_size.x / 2 , box.Min.y + box.Max.y + cur_offset_bottom , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_bottom += text_size.y + 6;
			break;
		case features::esp_placement_t::esp_placement_top:
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y - cur_offset_top - text_size.y - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , clr , true );
			render::gradient( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f + ( text_size.x + 4.0f ) * 0.5f , box.Min.y - cur_offset_top - text_size.y - 2.0f + ( text_size.y + 3.0f ) , ( text_size.x + 4.0f ) * 0.5f , 2.0f , clr , rgba( 0 , 0 , 0 , static_cast< int >( 33.0f * box_alpha ) ) , true );
			render::rect( box.Min.x + box.Max.x / 2 - text_size.x / 2 - 2.0f , box.Min.y - cur_offset_top - text_size.y - 2.0f , text_size.x + 4.0f , text_size.y + 3.0f , rgba( 0 , 0 , 0 , static_cast< int >( 72.0f * box_alpha ) ) );
			render::text( box.Min.x + box.Max.x / 2 - text_size.x / 2 , box.Min.y - cur_offset_top - text_size.y , as_str , _( "esp_font" ) , rgba( 255 , 255 , 255 , static_cast< int >( 255.0f * box_alpha ) ) , true );
			cur_offset_top += text_size.y + 6;
			break;
		}
	} break;
	case esp_type_t::esp_type_number: {
	} break;
	case esp_type_t::esp_type_flag: {
	} break;
	default: break;
	}
}

struct snd_info_t {
	int m_guid;
	void* m_hfile;
	int m_sound_src;
	int m_channel;
	int m_speaker_ent;
	float m_vol;
	float m_last_spatialized_vol;
	float m_rad;
	int pitch;
	vec3_t* m_origin;
	vec3_t* m_dir;
	bool m_update_positions;
	bool m_is_sentence;
	bool m_dry_mix;
	bool m_speaker;
	bool m_from_server;
	bool m_unk;
};

struct snd_data_t {
	snd_info_t* m_sounds;
	PAD( 8 );
	int m_count;
	PAD( 4 );
};

snd_data_t cached_data;

struct radar_player_t {
	vec3_t pos;
	vec3_t angle;
	vec3_t spotted_map_angle_related;
	uint32_t tab_related;
	PAD ( 12 );
	float spotted_time;
	float spotted_fraction;
	float time;
	PAD ( 4 );
	int player_index;
	int entity_index;
	PAD ( 4 );
	int health;
	char name [ 32 ];
	PAD ( 117 );
	bool spotted;
	PAD ( 138 );
};

struct hud_radar_t {
	PAD ( 332 );
	radar_player_t radar_info [ 65 ];
};

static void* find_hud_element ( const char* name ) {
	static auto hud = pattern::search ( _ ( "client.dll" ), _ ( "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 46 24" ) ).add ( 1 ).deref ( ).get< void* > ( );
	static auto find_hud_element_func = pattern::search ( _ ( "client.dll" ), _ ( "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28" ) ).get< void* ( __thiscall* )( void*, const char* ) > ( );
	return ( void* ) find_hud_element_func ( hud, name );
}

void features::esp::handle_dynamic_updates( ) {
	if ( !g::local )
		return;

	//send_data_to_clients ( );

	/* update with radar */
	auto radar_base = find_hud_element ( _ ( "CCSGO_HudRadar" ) );

	if ( radar_base ) {
		auto hud_radar = reinterpret_cast< hud_radar_t* > ( reinterpret_cast< uintptr_t >( radar_base ) - 20 );
	
		for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
			auto player = cs::i::ent_list->get< player_t* > ( i );
	
			if ( !player || !player->is_player ( ) || !player->alive ( ) )
				continue;

			auto& radar_info = hud_radar->radar_info [ i ];
	
			if ( radar_info.spotted_time > esp_data [ i ].m_spotted_time ) {
				if ( player->dormant ( ) ) {
					esp_data [ i ].m_dormant = true;
					esp_data [ i ].m_radar_pos = esp_data [ i ].m_sound_pos = hud_radar->radar_info [ i ].pos;
					esp_data [ i ].m_last_seen = cs::i::globals->m_curtime;
				}
				
				esp_data [ i ].m_spotted_time = radar_info.spotted_time;
			}
		}
	}

	/* update with sounds */
	//static auto get_active_sounds = pattern::search( _( "engine.dll" ) , _( "55 8B EC 83 E4 F8 81 EC 44 03 00 00 53 56" ) ).get< void( __thiscall* )( snd_data_t* ) >( );

	//memset( &cached_data , 0 , sizeof cached_data );
	//get_active_sounds( &cached_data );

	//if ( !cached_data.m_count )
	//	return;

	//for ( auto i = 0; i < cached_data.m_count; i++ ) {
	//	const auto sound = cached_data.m_sounds[ i ];

	//	if ( !sound.m_from_server || !sound.m_sound_src || sound.m_sound_src > 64 || !sound.m_origin || *sound.m_origin == vec3_t( 0.0f , 0.0f , 0.0f ) )
	//		continue;

	//	auto pl = cs::i::ent_list->get< player_t* >( sound.m_sound_src );

	//	if ( !pl || !pl->dormant( ) )
	//		continue;

	//	vec3_t end_pos = *sound.m_origin;

	//	trace_t tr;
	//	ray_t ray;

	//	trace_filter_t trace_filter;
	//	trace_filter.m_skip = pl;

	//	ray.init( *sound.m_origin + vec3_t( 0.0f , 0.0f , 2.0f ) , *sound.m_origin - vec3_t( 0.0f , 0.0f , 4096.0f ) );
	//	cs::i::trace->trace_ray( ray , mask_playersolid , &trace_filter , &tr );

	//	if ( !tr.is_visible( ) )
	//		end_pos = tr.m_endpos;

	//	if ( abs ( esp_data [ i ].m_spotted_time - cs::i::globals->m_curtime ) > 1.0f ) {
	//		esp_data [ pl->idx ( ) ].m_sound_pos = end_pos;
	//		esp_data [ pl->idx ( ) ].m_dormant = true;
	//		esp_data [ pl->idx ( ) ].m_last_seen = cs::i::globals->m_curtime;
	//	}
	//}
}

void features::esp::reset_dormancy( event_t* event ) {
	auto attacker = cs::i::ent_list->get< player_t* >( cs::i::engine->get_player_for_userid( event->get_int( _( "attacker" ) ) ) );
	auto victim = cs::i::ent_list->get< player_t* >( cs::i::engine->get_player_for_userid( event->get_int( _( "userid" ) ) ) );

	if ( !attacker || !victim || attacker != g::local || !g::local->is_enemy ( victim ) || ( victim->idx( ) <= 0 || victim->idx( ) > 64 ) )
		return;

	if ( !victim->dormant( ) ) {
		esp_data[ victim->idx( ) ].m_dormant = false;
		esp_data[ victim->idx( ) ].m_sound_pos = vec3_t( 0.f , 0.f , 0.f );
	}
}

void features::esp::render( ) {
	VMP_BEGINMUTATION ( );
	if ( g::round == round_t::starting ) {
		for ( auto& esp : esp_data ) {
			//esp.m_pl = nullptr;
			esp.m_dormant = true;
			esp.m_first_seen = esp.m_last_seen = 0.0f;
		}
	}

	if ( !g::local )
		return;

	for ( auto i = 1; i <= cs::i::globals->m_max_clients; i++ ) {
		auto e = cs::i::ent_list->get< player_t* >( i );

		if ( !e || !e->alive( ) || ( e == g::local && !cs::i::input->m_camera_in_thirdperson ) ) {
			esp_data[ i ].m_pl = nullptr;
			esp_data[ i ].m_health = 100.0f;
			continue;
		}

		features::visual_config_t visuals;
		if ( !get_visuals( e , visuals ) ) {
			esp_data[ i ].m_pl = nullptr;
			continue;
		}

		const auto idx = e->idx ( );

		vec3_t flb , brt , blb , frt , frb , brb , blt , flt;
		float left , top , right , bottom;
		
		auto abs_origin = ( e->bone_cache ( ) && !e->dormant ( ) ) ? e->bone_cache ( ) [ 1 ].origin ( ) : e->abs_origin ( );

		if ( e->dormant ( ) && esp_data[ idx ].m_sound_pos != vec3_t( 0.f , 0.f , 0.f ) )
			abs_origin = esp_data[ idx ].m_sound_pos;

		auto min = e->mins( ) + abs_origin;
		auto max = e->maxs( ) + abs_origin;

		if ( ( e->crouch_amount ( ) < 0.333f || e->crouch_amount ( ) == 1.0f ) && e->bone_cache ( ) && !e->dormant ( ) )
			max.z = e->bone_cache( )[ 8 ].origin( ).z + 12.0f;

		vec3_t points [ ] = {
			vec3_t( min.x, min.y, min.z ),
			vec3_t ( min.x, min.y, max.z ),
			vec3_t( min.x, max.y, min.z ),
			vec3_t ( min.x, max.y, max.z ),
			vec3_t( max.x, min.y, min.z ),
			vec3_t( max.x, min.y, max.z ),
			vec3_t ( max.x, max.y, min.z ),
			vec3_t ( max.x, max.y, max.z ),
		};

		if ( !cs::render::world_to_screen( flb , points[ 3 ] )
			|| !cs::render::world_to_screen( brt , points[ 5 ] )
			|| !cs::render::world_to_screen( blb , points[ 0 ] )
			|| !cs::render::world_to_screen( frt , points[ 4 ] )
			|| !cs::render::world_to_screen( frb , points[ 2 ] )
			|| !cs::render::world_to_screen( brb , points[ 1 ] )
			|| !cs::render::world_to_screen( blt , points[ 6 ] )
			|| !cs::render::world_to_screen( flt , points[ 7 ] ) ) {
			continue;
		}

		vec3_t arr [ ] = { flb, brt, blb, frt, frb, brb, blt, flt };

		left = flb.x;
		top = flb.y;
		right = flb.x;
		bottom = flb.y;

		for ( auto i = 1; i < 8; i++ ) {
			if ( left > arr[ i ].x )
				left = arr[ i ].x;

			if ( bottom < arr[ i ].y )
				bottom = arr[ i ].y;

			if ( right < arr[ i ].x )
				right = arr[ i ].x;

			if ( top > arr[ i ].y )
				top = arr[ i ].y;
		}

		const auto subtract_w = ( right - left ) / 5;
		const auto subtract_h = ( right - left ) / 16;

		right -= subtract_w / 2;
		left += subtract_w / 2;
		bottom -= subtract_h / 2;
		top += subtract_h / 2;

		esp_data[ idx ].m_pl = e;
		esp_data[ idx ].m_box.left = left;
		esp_data[ idx ].m_box.right = right;
		esp_data[ idx ].m_box.bottom = bottom;
		esp_data[ idx ].m_box.top = top;
		esp_data[ idx ].m_dormant = e->dormant( );
		esp_data [ idx ].m_scoped = e->scoped ( );
		esp_data [ idx ].m_radar_pos = e->origin ( );

		const auto clamped_health = std::clamp( static_cast< float >( e->health( ) ) , 0.0f , 100.0f );
		esp_data[ idx ].m_health += ( clamped_health - esp_data[ idx ].m_health ) * 2.0f * cs::i::globals->m_frametime;
		esp_data[ idx ].m_health = std::clamp( esp_data[ idx ].m_health , clamped_health , 100.0f );

		if ( g::round == round_t::starting )
			esp_data[ idx ].m_sound_pos = vec3_t( 0.0f, 0.0f, 0.0f );

		if ( !e->dormant( ) ) {
			esp_data[ idx ].m_sound_pos = abs_origin; // reset to our current origin since it will show our last sound origin
			esp_data[ idx ].m_pos = abs_origin;

			if ( esp_data[ idx ].m_first_seen == 0.0f )
				esp_data[ idx ].m_first_seen = cs::i::globals->m_curtime;

			esp_data[ idx ].m_last_seen = cs::i::globals->m_curtime;

			if ( e->weapon ( ) )
				esp_data [ idx ].m_weapon_name = cs::get_weapon_name ( e->weapon ( )->item_definition_index ( ) );
		}
		else {
			esp_data[ idx ].m_first_seen = 0.0f;
		}

		auto dormant_time = std::max< float >( 9.0f/*esp_fade_time*/ , 0.1f );

		if ( esp_data[ idx ].m_pl && std::fabsf( cs::i::globals->m_curtime - esp_data[ idx ].m_last_seen ) < dormant_time ) {
			auto calc_alpha = [ & ] ( float time , float fade_time , bool add = false ) {
				return ( std::clamp< float >( dormant_time - ( std::clamp< float >( add ? ( dormant_time - std::clamp< float >( std::fabsf( cs::i::globals->m_curtime - time ) , 0.0f , dormant_time ) ) : std::fabsf( cs::i::globals->m_curtime - time ) , std::max< float >( dormant_time - fade_time , 0.0f ) , dormant_time ) ) , 0.0f , fade_time ) / fade_time );
			};

			if ( !esp_data[ idx ].m_dormant )
				box_alpha = calc_alpha( esp_data[ idx ].m_first_seen , 0.6f , true );
			else
				box_alpha = calc_alpha( esp_data[ idx ].m_last_seen , 2.0f );

			std::string name = _( "" );

			player_info_t info;
			if ( cs::i::engine->get_player_info( idx, &info ) )
				name = info.m_name;

			if ( name.size( ) > 20 ) {
				name.resize( 19 );
				name += _( "..." );
			}

			if ( visuals.esp_box )
				draw_esp_box( left , top , right - left , bottom - top , esp_data[ idx ].m_dormant , visuals.box_color );

			cur_offset_left_height = 0;
			cur_offset_right_height = 0;
			cur_offset_left = 4;
			cur_offset_right = 4;
			cur_offset_bottom = 4;
			cur_offset_top = 4;

			ImVec4 esp_rect { left, top, right - left, bottom - top };

			if ( visuals.health_bar )
				draw_esp_widget( esp_rect , visuals.health_bar_color , esp_type_bar , esp_data [ idx ].m_health >= 100.0f ? false : visuals.value_text , visuals.health_bar_placement , esp_data[ idx ].m_dormant , esp_data[ idx ].m_health , 100.0 );

			if ( visuals.ammo_bar && e->weapon( ) && e->weapon( )->data( ) && e->weapon( )->ammo( ) != -1 )
				draw_esp_widget( esp_rect , visuals.ammo_bar_color , esp_type_bar , visuals.value_text , visuals.ammo_bar_placement , esp_data[ idx ].m_dormant , e->weapon( )->ammo( ) , e->weapon( )->data( )->m_max_clip );

			if ( visuals.desync_bar )
				draw_esp_widget( esp_rect , visuals.desync_bar_color , esp_type_bar , visuals.value_text , visuals.desync_bar_placement , esp_data[ idx ].m_dormant , e->desync_amount( ) , 58.0 );

			if ( visuals.nametag )
				draw_esp_widget( esp_rect , visuals.name_color , esp_type_text , visuals.value_text , visuals.nametag_placement , esp_data[ idx ].m_dormant , 0.0 , 0.0 , name );

			if ( visuals.weapon_name )
				draw_esp_widget( esp_rect , visuals.weapon_color , esp_type_text , visuals.value_text , visuals.weapon_name_placement , esp_data[ idx ].m_dormant , 0.0 , 0.0 , esp_data[ idx ].m_weapon_name );

			if ( visuals.fakeduck_flag && esp_data[ idx ].m_fakeducking )
				draw_esp_widget( esp_rect , visuals.fakeduck_color , esp_type_text , visuals.value_text , visuals.fakeduck_flag_placement , esp_data[ idx ].m_dormant , 0.0 , 0.0 , _( "FD" ) );

			if ( visuals.reloading_flag && esp_data[ idx ].m_reloading )
				draw_esp_widget( esp_rect , visuals.reloading_color , esp_type_text , visuals.value_text , visuals.reloading_flag_placement , esp_data[ idx ].m_dormant , 0.0 , 0.0 , _( "Reload" ) );

			if ( visuals.fatal_flag && esp_data[ idx ].m_fatal )
				draw_esp_widget( esp_rect , visuals.fatal_color , esp_type_text , visuals.value_text , visuals.fatal_flag_placement , esp_data[ idx ].m_dormant , 0.0 , 0.0 , _( "Fatal" ) );
			
			if ( visuals.zoom_flag && esp_data [ idx ].m_scoped )
				draw_esp_widget ( esp_rect, visuals.zoom_color, esp_type_text, visuals.value_text, visuals.zoom_flag_placement, esp_data [ idx ].m_dormant, 0.0, 0.0, _ ( "Zoom" ) );

			const auto& anim_info = anims::anim_info [ idx ];

			if ( g::local->is_enemy ( e ) && !anim_info.empty( ) && anim_info.front( ).m_lby_update )
				draw_esp_widget ( esp_rect, { 0.0f, 1.0f, 0.0f, 1.0f }, esp_type_text, visuals.value_text, visuals.zoom_flag_placement, esp_data [ idx ].m_dormant, 0.0, 0.0, _ ( "LBY" ) );
			
			//if ( anims::resolver::is_spotted(g::local, e) )
			//	dbg_print ( _("SPOTTED!! %d\n"), cs::i::globals->m_tickcount );

			//const auto eyes_max = e->eyes( );
			//const auto eyes_fwd = cs::angle_vec( vec3_t( 0.0f, e->angles ( ).y, 0.0f ) ).normalized ( );
			//const auto eyes_right = eyes_fwd.cross_product ( vec3_t ( N ( 0 ), N ( 0 ), N ( 1 ) ) );
			//
			//auto left_head = eyes_max + eyes_fwd * N ( 30 ) + eyes_right * N ( 10 );
			//auto center_head = eyes_max + eyes_fwd * N ( 30 );
			//auto right_head = eyes_max + eyes_fwd * N ( 30 ) - eyes_right * N ( 10 );
			//
			//vec3_t mid_wrld, left_wrld, right_wrld;
			//if ( cs::render::world_to_screen ( mid_wrld, center_head ) && cs::render::world_to_screen ( left_wrld, left_head ) && cs::render::world_to_screen ( right_wrld, right_head ) ) {
			//	render::circle ( left_wrld.x, left_wrld.y, 2.0f, 6, rgba ( 255, 0, 0, 255 ) ); /* left red */
			//	render::circle ( mid_wrld.x, mid_wrld.y, 2.0f, 6, rgba ( 0, 255, 0, 255 ) ); /* green middle */
			//	render::circle ( right_wrld.x, right_wrld.y, 2.0f, 6, rgba ( 0, 0, 255, 255 ) ); /* right blue */
			//}

			//if ( e == g::local && g::local )
			//	dbg_print ( _("desync_amount: %.2f\n"), g::local->desync_amount() );

			if ( e != g::local && g::local->is_enemy ( e ) ) {
				draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ idx ].m_dormant, 0.0, 0.0, anims::resolver_mode_names [ anims::resolver_mode [ idx - 1 ] ] );
			}

			//if ( e != g::local && g::local->is_enemy(e) ) {
			//	std::string output = "animlayer 3:\n";
			//	output.append ( "weight: " + std::to_string ( anims::layer3 [ idx - 1 ].m_weight ) + ":\n" );
			//	output.append ( "cycle: " + std::to_string ( anims::layer3 [ idx - 1 ].m_cycle ) + ":\n" );
			//	output.append ( "rate: " + std::to_string ( anims::layer3 [ idx - 1 ].m_playback_rate ) + ":\n" );
			//
			//	draw_esp_widget ( esp_rect, visuals.weapon_color, esp_type_text, visuals.value_text, esp_placement_right, esp_data [ idx ].m_dormant, 0.0, 0.0, output );
			//}

			// Angle indicators
			auto angle_to_dir = [ ] ( float angle_deg ) {
				float s, c;
				cs::sin_cos ( cs::deg2rad ( angle_deg ), &s, &c );
				return vec3_t ( c, s, 0.0f );
			};

			if ( g::local->is_enemy ( e ) && anims::last_moving_lby_time [ idx - 1 ] != std::numeric_limits<float>::max ( ) ) {
				vec3_t v0, v1, v2, v3;
				auto last_moving_lby = abs_origin + angle_to_dir ( anims::last_moving_lby [ idx - 1 ] ) * 35.0f;
				auto lby = abs_origin + angle_to_dir ( anims::last_lby [ idx - 1 ] ) * 35.0f;
				auto last_freestanding = abs_origin + angle_to_dir ( anims::last_freestanding [ idx - 1 ] ) * 35.0f;

				if ( cs::render::world_to_screen ( v0, abs_origin )
					&& cs::render::world_to_screen ( v1, last_moving_lby )
					&& cs::render::world_to_screen ( v2, lby )
					&& cs::render::world_to_screen ( v3, last_freestanding ) ) {
					render::line ( v0.x, v0.y, v1.x, v1.y, 0xFF0000FF, 2.0f );
					render::text ( v1.x, v1.y, "move", "esp_font", 0xFFFFFFFF, true );

					render::line ( v0.x, v0.y, v2.x, v2.y, 0xFF00FFFF, 2.0f );
					render::text ( v2.x, v2.y, "lby", "esp_font", 0xFFFFFFFF, true );

					render::line ( v0.x, v0.y, v3.x, v3.y, 0xFFFF0000, 2.0f );
					render::text ( v3.x, v3.y, "edge", "esp_font", 0xFFFFFFFF, true );
				}
			}
		}
	}
	VMP_END ( );
}