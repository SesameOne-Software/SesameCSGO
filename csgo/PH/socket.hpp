#pragma once
#include <mutex>

namespace client {
	bool create_socket(const char* url, const char* port);
	bool send_buffer(const char* buffer, int bufflen);
	std::string read_buffer();
	void close_connection();
}