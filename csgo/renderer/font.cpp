#include <shlobj_core.h>
#include <vector>

#include "../sdk/sdk.hpp"
#include "font.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h"

ID3DXSprite *font_sprite = nullptr;

struct font_vertex_t
{
	float x, y, z, rhw;
	uint32_t color;
	float tx, ty;
};

std::optional<truetype::font> truetype::create_font(const std::vector<uint8_t> &font_data, std::string_view font_name, float size)
{
	if (stbtt_fontinfo font_info; stbtt_InitFont(&font_info, font_data.data(), 0))
		return font{font_name.data(), size, font_info};

	return {};
}

IDirect3DTexture9 *truetype::font::create_texture(const std::string &string)
{
	constexpr auto bitmap_width = 512;

	auto bitmap = std::make_unique<std::uint8_t[]>(bitmap_width * static_cast<int>(size + 0.5f));

	auto scale = stbtt_ScaleForPixelHeight(&font_info, size);

	auto x = 0, ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &lineGap);

	ascent *= scale;
	descent *= scale;

	for (auto i = 0; i < string.size(); ++i)
	{
		auto c_x1 = 0, c_y1 = 0, c_x2 = 0, c_y2 = 0;
		stbtt_GetCodepointBitmapBox(&font_info, string[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

		auto y = ascent + c_y1;

		auto byteOffset = x + (y * bitmap_width);
		stbtt_MakeCodepointBitmap(&font_info, bitmap.get() + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, string[i]);

		auto ax = 0;
		stbtt_GetCodepointHMetrics(&font_info, string[i], &ax, 0);
		x += ax * scale;

		auto kern = stbtt_GetCodepointKernAdvance(&font_info, string[i], string[i + 1]);
		x += kern * scale;
	}

	IDirect3DTexture9 *texture = nullptr;

	if (D3DXCreateTexture(csgo::i::dev, bitmap_width, static_cast<int>(size + 0.5f), 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture) < 0)
		return nullptr;

	D3DLOCKED_RECT rect;

	if (texture->LockRect(0, &rect, nullptr, 0) < 0)
		return nullptr;

	memcpy(rect.pBits, bitmap.get(), bitmap_width * static_cast<int>(size + 0.5f));

	texture->UnlockRect(0);

	return texture;
}

void truetype::font::draw_text(float x, float y, const std::string &string, uint32_t color, text_flags_t flags)
{
	auto texture = create_texture(string);

	if (!texture || !font_sprite || font_sprite->Begin(D3DXSPRITE_ALPHABLEND) < 0)
	{
		if (texture)
		{
			texture->Release();
			texture = nullptr;
		}

		return;
	}

	font_sprite->Draw(texture, nullptr, nullptr, &D3DXVECTOR3{x, y, 0.0f}, color);
	font_sprite->End();

	texture->Release();
	texture = nullptr;
}

void truetype::begin()
{
	D3DXCreateSprite(csgo::i::dev, &font_sprite);
}

void truetype::end()
{
	if (font_sprite)
	{
		font_sprite->Release();
		font_sprite = nullptr;
	}
}