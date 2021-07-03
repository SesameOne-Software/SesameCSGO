#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <mutex>

#include "socket.hpp"

#include "Base64.h"
#include "../cjson/cJSON.h"
#include "AES/AES.h"
#include "picosha2/picosha2.h"
#include "../security/xorstr.hpp"

#include <string>

/*
	This heartbeat will be weak depending on what methods you use. 
	Creating a separate thread on a loop is very weak. 
	It would be good to Xor all strings, virtualize all functions, and mutate the ph_heartbeat::on_fail() function.

	Note: All functions are forced to be inlined because it makes it harder to read, especially with good virtualization. This may decrease performance, but helps with security.
	You can always remove __forceinline from any functions without causing issues to the functionality.
*/

namespace ph_heartbeat {
	std::mutex ph_heartbeat_mutex;
	std::mutex ph_heartbeat_user_mutex;
	std::mutex ph_heartbeat_session_mutex;

	/*
		Set this value to 0 to not print any information about the heartbeat.
	*/

	//#define PH_HEARTBEAT_DEBUG 1

	/*
		DO NOT change these values unless advised to by the owner of Project Hades.
	*/

	#define PH_HEARTBEAT_TASK 30

	#define PH_HEARTBEAT_INIT_TASK 40

	/*
		Set PH_SECONDS_INTERVAL to a value LESS than the max interval of seconds you set for the heartbeat of this module
		This value won't be until you use it in your timer for sending heartbeats. For milliseconds, multiply this value by 1000
	*/

	constexpr auto PH_SECONDS_INTERVAL = 3;

	/*
		Insert the Application ID, found on the panel for Project Hades, in PH_APPLICATION_ID
		It is recommended to Xor this string at both runtime and compilation
		The reason you should Xor it both ways is because the DLL can be dumped from the process memory before entrypoint is called and the string won't be xor'd because if it's only xor'd at runtime
	*/

	//std::string PH_APPLICATION_ID = std::string(_("b63f52e4e746bd22e69e86226aba5dc8"));

	/*
		This structure is the data that will be passed from the loader to the entrypoint.
	*/

	typedef struct heartbeat_info {
		#ifdef _WIN64
			std::uint64_t image_base;
		#else
			std::uint32_t image_base;
		#endif

		const char* heartbeat_token_hashed;
		const char* host;
		const char* port;
	};

	static std::string session_token, username;

	static const char* heartbeat_token_hashed;

	__forceinline static std::string& get_username() {
		ph_heartbeat::ph_heartbeat_user_mutex.lock();
		static auto user = ph_heartbeat::username;
		ph_heartbeat::ph_heartbeat_user_mutex.unlock();

		return user;
	}

	/*
		This code will run when either it fails to send the heartbeat or the server doesn't validate the heartbeat.
	*/

	__forceinline static void on_fail() {
		#ifdef PH_HEARTBEAT_DEBUG
			MessageBoxA(0, _("Failed to verify session. Ending session..."), 0, 0);
		#endif

		client::close_connection();
		exit(0);
	}

	static std::vector<unsigned char> aes_key(picosha2::k_digest_size);

	/*
		CALL ph_heartbeat::initialize_heartbeat BEFORE CALLING THIS FUNCTION
		Call this function wherever you want to send a heartbeat request

		Remember: This function should be virtualized for the best protection
		
		***YOU DON'T NEED TO CALL THIS AFTER INITIALIZATION IF YOU ARE ALREADY CALLING IT IN A HOOK***
		However, calling it in multiple hooks or threads will help if someone finds one thread that it runs in
	*/

	__forceinline static void send_heartbeat(int task = PH_HEARTBEAT_TASK) {
		AES aes(256);

		cJSON* heartbeat_json = cJSON_CreateObject();

		switch (task) {
			case PH_HEARTBEAT_INIT_TASK: {
				ph_heartbeat_mutex.lock();

				std::string key_str = std::string ( _ ( "b63f52e4e746bd22e69e86226aba5dc8" ) ) + ph_heartbeat::heartbeat_token_hashed;

				picosha2::hash256(key_str.begin(), key_str.end(), aes_key.begin(), aes_key.end());

				std::string app_id_hash_str;
				picosha2::hash256_hex_string( std::string ( _ ( "b63f52e4e746bd22e69e86226aba5dc8" ) ), app_id_hash_str);

				std::string token_hashed = ph_heartbeat::heartbeat_token_hashed;

				ph_heartbeat_mutex.unlock();

				cJSON* appid_hashed_json = cJSON_CreateString(app_id_hash_str.c_str());
				cJSON_AddItemToObject(heartbeat_json, _("h1"), appid_hashed_json); // Xor the string "h1" for more protection

				cJSON* token_hashed_json = cJSON_CreateString(token_hashed.c_str());
				cJSON_AddItemToObject(heartbeat_json, _("h2"), token_hashed_json); // Xor the string "h2" for more protection

				token_hashed.clear(); // Removing data from token_hashed string
			} break;

			case PH_HEARTBEAT_TASK: {
				ph_heartbeat::ph_heartbeat_session_mutex.lock();
				auto cipher_vec = std::vector<unsigned char>(session_token.data(), session_token.data() + session_token.size());

				std::default_random_engine generator;
				std::uniform_int_distribution<int> distribution(0, 255);
				std::vector<unsigned char> initialization_vector(0);
				for (int i = 0; i < 16; i++)
					initialization_vector.push_back(distribution(generator));

				unsigned int outlen = 0;

				auto encrypted_cipher = aes.EncryptCFB(&cipher_vec[0], session_token.length(), &aes_key[0], &initialization_vector[0], outlen);

				auto encoded_cipher = macaron::Base64::Encode(encrypted_cipher, outlen);

				ph_heartbeat::ph_heartbeat_session_mutex.unlock();

				cJSON* cipher_json = cJSON_CreateString(encoded_cipher.c_str());
				cJSON_AddItemToObject(heartbeat_json, _("cipher"), cipher_json); // Xor the string "cipher" for more protection

				std::string iv_hex_str = picosha2::bytes_to_hex_string(initialization_vector);
				cJSON* iv_json = cJSON_CreateString(iv_hex_str.c_str());
				cJSON_AddItemToObject(heartbeat_json, _("iv"), iv_json); // Xor the string "iv" for more protection
			} break;
		}

		cJSON* task_json = cJSON_CreateNumber(task);
		cJSON_AddItemToObject(heartbeat_json, _("task"), task_json); // Xor the string "task" for more protection

		const auto json_string = cJSON_PrintUnformatted(heartbeat_json);
		cJSON_Delete(heartbeat_json);

		if (!client::send_buffer(json_string, strlen(json_string))) {
			ph_heartbeat::on_fail();
			return;
		}

		auto json_return_buffer = client::read_buffer();

		if (json_return_buffer.empty()) {
			ph_heartbeat::on_fail();
			return;
		}

		cJSON* json_return = cJSON_Parse(json_return_buffer.c_str());

		json_return_buffer.clear();

		cJSON* cipher_value = cJSON_GetObjectItemCaseSensitive(json_return,_( "cipher")); // Xor the string "cipher" for more protection

		cJSON* iv_value = cJSON_GetObjectItemCaseSensitive(json_return, _("iv")); // Xor the string "iv" for more protection

		int iv_out_len = 0;
		auto iv_array = macaron::Base64::decode(iv_value->valuestring);
		
		auto cipher_ret = macaron::Base64::decode(cipher_value->valuestring);

		ph_heartbeat::ph_heartbeat_session_mutex.lock();

		std::string decrypted_cipher = reinterpret_cast<const char*>(aes.DecryptCFB(&cipher_ret[0], cipher_ret.size(), &aes_key[0], &iv_array[0]));

		ph_heartbeat::ph_heartbeat_session_mutex.unlock();

		auto decrypted_fixed = decrypted_cipher.substr(0, std::string_view(decrypted_cipher).find_first_of('}') + 1); // Decryption has trailing random characters for some reason. Added a Ghetto fix for now.
		decrypted_cipher.clear();

		cJSON_Delete(json_return);

		cJSON* cipher_ret_json = cJSON_Parse(decrypted_fixed.c_str());

		decrypted_fixed.clear();

		cJSON* success_value = cJSON_GetObjectItemCaseSensitive(cipher_ret_json, _("success")); // Xor the string "success" for more protection

		if (success_value->type == cJSON_False) {
			ph_heartbeat::on_fail();
			return;
		}

		ph_heartbeat::ph_heartbeat_session_mutex.lock();

		cJSON* session_value = cJSON_GetObjectItemCaseSensitive(cipher_ret_json, _("session")); // Xor the string "session" for more protection
		ph_heartbeat::session_token = session_value->valuestring;

		ph_heartbeat::ph_heartbeat_session_mutex.unlock();
		
		if (task == PH_HEARTBEAT_INIT_TASK) {
			ph_heartbeat::ph_heartbeat_user_mutex.lock();

			if (ph_heartbeat::username.empty()) {
				cJSON* user_value = cJSON_GetObjectItemCaseSensitive(cipher_ret_json, _("user")); // Xor the string "user" for more protection
				ph_heartbeat::username = user_value->valuestring;
			}

			ph_heartbeat::ph_heartbeat_user_mutex.unlock();
		}

		cJSON_Delete(cipher_ret_json);

		#ifdef PH_HEARTBEAT_DEBUG
			ph_heartbeat::ph_heartbeat_user_mutex.lock();
			MessageBoxA(0, std::string(_("Heartbeat was successful! Hello, ") + ph_heartbeat::username).c_str(), 0, 0);
			ph_heartbeat::ph_heartbeat_user_mutex.unlock();
		#endif
	}

	/*
		Call this inside your entrypoint on attach to initialize the heartbeat session
	*/

	__forceinline static void initialize_heartbeat(ph_heartbeat::heartbeat_info* in_info) {
		heartbeat_token_hashed = in_info->heartbeat_token_hashed;

		if (!client::create_socket(in_info->host, in_info->port))
			ph_heartbeat::on_fail();

		ph_heartbeat::send_heartbeat(PH_HEARTBEAT_INIT_TASK);

		VirtualFree(in_info, 0, MEM_RELEASE); // Releasing data so token hashed is no longer in memory
	}
}