#ifndef _MACARON_BASE64_H_
#define _MACARON_BASE64_H_

/**
 * The MIT License (MIT)
 * Copyright (c) 2016 tomykaira
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string>

namespace macaron {
	inline const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	class Base64 {
	public:

		static std::string Encode(uint8_t* data, size_t in_len) {
			static constexpr char sEncodingTable[] = {
			  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			  'w', 'x', 'y', 'z', '0', '1', '2', '3',
			  '4', '5', '6', '7', '8', '9', '+', '/'
			};

			size_t out_len = 4 * ((in_len + 2) / 3);
			std::string ret(out_len, '\0');
			size_t i;
			char* p = const_cast<char*>(ret.c_str());

			for (i = 0; i < in_len - 2; i += 3) {
				*p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
				*p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
				*p++ = sEncodingTable[((data[i + 1] & 0xF) << 2) | ((int)(data[i + 2] & 0xC0) >> 6)];
				*p++ = sEncodingTable[data[i + 2] & 0x3F];
			}
			if (i < in_len) {
				*p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
				if (i == (in_len - 1)) {
					*p++ = sEncodingTable[((data[i] & 0x3) << 4)];
					*p++ = '=';
				}
				else {
					*p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
					*p++ = sEncodingTable[((data[i + 1] & 0xF) << 2)];
				}
				*p++ = '=';
			}

			return ret;
		}

		static unsigned int pos_of_char(const unsigned char chr) {
			//
			// Return the position of chr within base64_encode()
			//

			if (chr >= 'A' && chr <= 'Z') return chr - 'A';
			else if (chr >= 'a' && chr <= 'z') return chr - 'a' + ('Z' - 'A') + 1;
			else if (chr >= '0' && chr <= '9') return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
			else if (chr == '+' || chr == '-') return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
			else if (chr == '/' || chr == '_') return 63; // Ditto for '/' and '_'
			else
				//
				// 2020-10-23: Throw std::exception rather than const char*
				//(Pablo Martin-Gomez, https://github.com/Bouska)
				//
				//throw std::runtime_error("Input is not valid base64-encoded data.");
				return 0;
		}


		static inline bool is_base64(unsigned char c) {
			return (isalnum(c) || (c == '+') || (c == '/'));
		}

		static std::vector<unsigned char> decode(std::string const& encoded_string) {
			int in_len = encoded_string.size();
			int i = 0;
			int j = 0;
			int in_ = 0;
			unsigned char char_array_4[4], char_array_3[3];
			std::vector<unsigned char> ret;

			while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
				char_array_4[i++] = encoded_string[in_]; in_++;
				if (i == 4) {
					for (i = 0; i < 4; i++)
						char_array_4[i] = base64_chars.find(char_array_4[i]);

					char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
					char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
					char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

					for (i = 0; (i < 3); i++)
						ret.push_back(char_array_3[i]);
					i = 0;
				}
			}

			if (i) {
				for (j = i; j < 4; j++)
					char_array_4[j] = 0;

				for (j = 0; j < 4; j++)
					char_array_4[j] = base64_chars.find(char_array_4[j]);

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
			}

			return ret;
		}
	};
}

#endif /* _MACARON_BASE64_H_ */