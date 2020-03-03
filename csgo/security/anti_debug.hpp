#pragma once

#include <Windows.h>
#include <Winternl.h>
#include <string>

#pragma warning( disable : 4091)

//main namespace for security
namespace security {
	//internal (used by the security itself, no need to be used outside of namespace)
	namespace internal {
		__forceinline void to_lower( unsigned char* input );
		__forceinline const wchar_t* get_string( int index );

		//dynamically resolved functions
		typedef NTSTATUS( __stdcall* _NtQueryInformationProcess )( _In_ HANDLE, _In_  unsigned int, _Out_ PVOID, _In_ ULONG, _Out_ PULONG );
		typedef NTSTATUS( __stdcall* _NtSetInformationThread )( _In_ HANDLE, _In_ THREAD_INFORMATION_CLASS, _In_ PVOID, _In_ ULONG );

		//enum for the results of the antidebugger
		enum debug_results
		{
			//nothing was caught, value = 0
			none = 0x0000,

			//something caught in memory (0x1000 - 0x1009)
			being_debugged_peb = 0x1000,
			remote_debugger_present = 0x1001,
			debugger_is_present = 0x1002,
			dbg_global_flag = 0x1003,
			nt_query_information_process = 0x0004,
			find_window = 0x1005,
			output_debug_string = 0x1006,
			nt_set_information_thread = 0x1007,
			debug_active_process = 0x1008,
			write_buffer = 0x1009,

			//something caught in exceptions (0x2000 - 0x2005)
			close_handle_exception = 0x2000,
			single_step = 0x2001,
			int_3_cc = 0x2002,
			int_2 = 0x2003,
			prefix_hop = 0x2004,
			debug_string = 0x2005,

			//something caught with timings (0x3000 - 0x3002)
			rdtsc = 0x3000,
			query_performance_counter = 0x3001,
			get_tick_count = 0x3002,

			//something caught in cpu (0x4000 - 0x4001)
			hardware_debug_registers = 0x4000,
			mov_ss = 0x4001,

			//virtualization (0x5000 - 0x5003)
			check_cpuid = 0x5000,
			check_registry = 0x5001,
			vm = 0x5002,
		};

		namespace memory {
			__forceinline int being_debugged_peb( );
			__forceinline int remote_debugger_present( );
			__forceinline int check_window_name( );
			__forceinline int is_debugger_present( );
			__forceinline int nt_global_flag_peb( );
			__forceinline int nt_query_information_process( );
			__forceinline int nt_set_information_thread( );
			__forceinline int debug_active_process( );
			__forceinline int write_buffer( );
		}

		namespace exceptions {
			__forceinline int close_handle_exception( );
			__forceinline int single_step_exception( );
			__forceinline int int_3( );
			__forceinline int int_2d( );
			__forceinline int prefix_hop( );
			__forceinline int debug_string( );
		}

		namespace timing {
			__forceinline int rdtsc( );
			__forceinline int query_performance_counter( );
			__forceinline int get_tick_count( );
		}

		namespace cpu {
			__forceinline int hardware_debug_registers( );
			__forceinline int mov_ss( );
		}

		namespace virtualization {
			__forceinline int check_cpuid( );
			__forceinline int check_registry( );
			__forceinline int vm( );
		}
	}

	extern __forceinline internal::debug_results check_security( );
}