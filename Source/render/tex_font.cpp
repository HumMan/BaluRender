#include "../../Include/baluRender.h"

using namespace BaluRender;

#include "../baluRenderCommon.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H
#include FT_CACHE_CHARMAP_H


#include <IL/ilut.h>

#include<algorithm>

class TGlyphDesc
{
public:
	TVec2i Size;       // Size of glyph
	TVec2i Bearing;    // Offset from baseline to left/top of glyph
	int     Advance;    // Offset to advance to next glyph
	unsigned char* image;
	int char_id;
	TGlyphDesc(TVec2i Size, TVec2i Bearing, int Advance, const unsigned char* image, int char_id, bool mirror_y)
	{
		this->char_id = char_id;
		//бордюр между символами
		Size += TVec2i(2, 2);
		this->Size = Size;
		this->Bearing = Bearing;
		this->Advance = Advance;
		size_t size = Size[0] * Size[1];
		this->image = new unsigned char[size];

		for (int y = 0; y < Size[1]; y++)
		{
			for (int x = 0; x < Size[0]; x++)
			{
				if (x != 0 && x != Size[0] - 1 && y != 0 && y != Size[1] - 1)
				{
					if (mirror_y)
						this->image[Size[0] * (Size[1] - 1 - y) + x] = image[(Size[0] - 2) * (y - 1) + x - 1];
					else
						this->image[Size[0] * y + x] = image[(Size[0] - 2) * (y - 1) + x - 1];
				}
				else
					this->image[Size[0] * y + x] = 0;
			}
		}

	}
};

class TGlyph
{
public:
	TVec2i pos, size;
	TQuad<float, 2> tex_coords; // обход прямоугольника CCW
	int glyph_desc_id;
	TGlyph(TVec2i use_pos, TVec2i use_size, int use_glyph_desc_id) :pos(use_pos), size(use_size), glyph_desc_id(use_glyph_desc_id)
	{
	}
	TGlyph() {}
	void CalcTextCoord(int tex_size)
	{
		TVec2 t;//TODO походу надо зеркалировать
		TVec2i temp;
		temp = (pos + size.ComponentMul(TVec2i(0, 0)));
		t = TVec2(static_cast<float>(temp[0]), static_cast<float>(temp[1]))*(1.0f / tex_size);
		tex_coords[1] = TVec2(t[0], t[1]);

		temp = (pos + size.ComponentMul(TVec2i(0, 1)));
		t = TVec2(temp[0], temp[1])*(1.0f / static_cast<float>(tex_size));
		tex_coords[0] = TVec2(t[0], t[1]);

		temp = (pos + size.ComponentMul(TVec2i(1, 1)));
		t = TVec2(temp[0], temp[1])*(1.0f / static_cast<float>(tex_size));
		tex_coords[3] = TVec2(t[0], t[1]);

		temp = (pos + size.ComponentMul(TVec2i(1, 0)));
		t = TVec2(temp[0], temp[1])*(1.0f / static_cast<float>(tex_size));
		tex_coords[2] = TVec2(t[0], t[1]);
	}
};

class TGlyphPacker
{
private:

	struct TRow
	{
		std::vector<TGlyph> glyph;
		int curr_width;
		TRow() :curr_width(0) {}
		TVec2i PushGlyph(int curr_heigth, TVec2i size, int use_glyph_desc_id)
		{
			glyph.push_back(TGlyph(TVec2i(curr_width, curr_heigth), size, use_glyph_desc_id));
			curr_width += size[0];
			return glyph.back().pos;
		}
	};
	int max_heigh;
	int curr_height;
	std::vector<TRow> rows;
	std::vector<TGlyphDesc> glyphs;

	int tex_size = 0;

	struct less_than_key
	{
		inline bool operator() (const TGlyphDesc& struct1, const TGlyphDesc& struct2)
		{
			return (struct1.Size[1] < struct2.Size[1]);
		}
	};

	TVec2i PushGlyph(TVec2i size, int use_glyph_desc_id)
	{
		//влезают ли символы в текущий row
		if (rows.size() == 0 || rows.back().curr_width + size[0] > tex_size)
		{
			if (rows.size() != 0)
			{
				curr_height += max_heigh;
				max_heigh = 0;
			}
			rows.emplace_back();
		}
		//вычисляем символ с наибольшей высотой
		if (size[1] > max_heigh)
			max_heigh = size[1];
		return rows.back().PushGlyph(curr_height, size, use_glyph_desc_id);
	}

	template<typename T>
	int find(const T arr[], size_t len, T seek)
	{
		for (size_t i = 0; i < len; ++i)
		{
			if (arr[i] == seek) return i;
		}
		return -1;
	}

	template<typename T>
	void CopyGlyph(int width, int height, int left, int bottom, T* source, T* dest)
	{
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
			{
				int p = (x + left) + tex_size*(y + bottom);

				dest[p * 2 + 0] = source[x + width*y];
				dest[p * 2 + 1] = source[x + width*y];
			}
	}

	void* FillMap()
	{
		unsigned char* map = new unsigned char[sqr(tex_size) * 2];
		memset(map, 0, sqr(tex_size) * 2);
		for (auto& r : rows)
		{
			for (auto& g : r.glyph)
			{
				CopyGlyph(g.size[0], g.size[1], g.pos[0], g.pos[1], glyphs[g.glyph_desc_id].image, map);
			}
		}
		return map;
	}

	bool TryPackGlyphs()
	{
		rows.clear();
		max_heigh = 0;
		curr_height = 0;
		for (size_t i = 0; i < glyphs.size(); i++)
		{
			PushGlyph(glyphs[i].Size, i);
			if (curr_height + max_heigh > tex_size)
				return false;
		}
		return true;
	}

public:

	const char* all_chars;

	TGlyphPacker() :max_heigh(0), curr_height(0), all_chars(nullptr) {}
	TGlyphPacker(const char* use_all_chars) :max_heigh(0), curr_height(0), all_chars(use_all_chars) {}

	void ClearImages()
	{
		for (auto& v : glyphs)
		{
			delete v.image;
			v.image = nullptr;
		}
	}

	TGlyphDesc GetDescById(int id)
	{
		return glyphs[id];
	}

	TGlyph GetGlyphByChar(char c)
	{
		size_t index;
		if (all_chars != nullptr)
			index = find(all_chars, strlen(all_chars), c);
		else
			index = c;

		auto curr_row_index = 0;
		while (rows[curr_row_index].glyph.size() <= index)
		{
			index -= rows[curr_row_index].glyph.size();
			curr_row_index++;
		}
		return rows[curr_row_index].glyph[index];
	}

	void PushGlyphDesc(TGlyphDesc desc)
	{
		glyphs.push_back(desc);
	}

	void* BuildMap()
	{
		//сортируем по высоте символа
		//TODO - поиск в GetGlyphByChar учесть
		//std::sort(glyphs.begin(), glyphs.end(), less_than_key());


		tex_size = 256;
		while (!TryPackGlyphs())
		{
			tex_size *= 2;
		}

		auto result = FillMap();
		for (auto& r : rows)
		{
			for (auto& g : r.glyph)
			{
				g.CalcTextCoord(tex_size);
			}
		}
		return result;
	}

	int GetTexSize()
	{
		return tex_size;
	}

};

class TTexFontDesc
{
public:
	int texture;

	std::shared_ptr<TGlyphPacker> glyphs;
	TTexFontDesc()
	{
		texture = -1;
	}
};


class TBaluRender::TTexFont::TTexFontPrivate
{
public:
	std::vector<TTexFontDesc> fonts;

	std::vector<TQuad<float, 3>> quads;
	std::vector<TQuad<float, 2>> tex_coords;

	TTexFontPrivate()
	{

	}
};


#if PLATFORM==PLATFORM_WIN32
#  define FREETYPE_PLATFORM TT_PLATFORM_MICROSOFT
#  define FREETYPE_ENCODING TT_MS_ID_UNICODE_CS
#elif PLATFORM==PLATFORM_LINUX // не проверено - в документации по этому поводу как-то коряво написано
#  define FREETYPE_PLATFORM TT_PLATFORM_APPLE_UNICODE
#  define FREETYPE_ENCODING TT_APPLE_ID_DEFAULT
#endif

TBaluRender::TTexFont::TTexFont(TBaluRender* r)
{
	this->r = r;
	this->tex_font = new TBaluRender::TTexFont::TTexFontPrivate();
}

TBaluRender::TTexFont::~TTexFont()
{
	delete this->tex_font;
}

//const char text[] = " _abcdefghigklmnopqrstuvwxyzABCDEFGHIGKLMNOPQRSTUVWXYZ1234567890-=_+//*!\"№;%:?*()";

TTexFontId TBaluRender::TTexFont::Create(const char* font_path, unsigned int pixel_height)
{
	FT_Error error;
	FT_Library library;
	error = FT_Init_FreeType(&library);
	FT_Face face = nullptr;

	error = FT_New_Face(library, font_path, 0, &face);

	error = FT_Set_Pixel_Sizes(
		face,   /* handle to face object */
		0,      /* pixel_width           */
		pixel_height);   /* pixel_height          */

	{
		FT_CharMap  found = 0;
		FT_CharMap  charmap;
		int         n;
		for (n = 0; n < face->num_charmaps; n++)
		{
			charmap = face->charmaps[n];
			if (charmap->platform_id == FREETYPE_PLATFORM &&
				charmap->encoding_id == FREETYPE_ENCODING)
			{
				found = charmap;
				break;
			}
		}
		error = FT_Set_Charmap(face, found);
	}

	//int len = strlen(text);

	TGlyphPacker* glyph_packer = new TGlyphPacker();

	for (int i = 0; i < 127; i++)
		//for (int k = 0; k < len; k++)
	{
		//auto i = text[k];
		error = FT_Load_Char(face, (unsigned char)i, FT_LOAD_RENDER);

		glyph_packer->PushGlyphDesc(TGlyphDesc(
			TVec2i(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			TVec2i(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x,
			face->glyph->bitmap.buffer, i, true));

		//FT_UInt glyph_index = FT_Get_Char_Index(face, i);

		//error = FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT);

		//FT_Glyph glyph;
		//FT_Get_Glyph(face->glyph, &glyph);
		//error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
		//FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
		//FT_Bitmap &bitmap = bitmap_glyph->bitmap;
		//int width = bitmap.width;
		//int height = bitmap.rows;

		//glyph_packer->PushGlyphDesc(TGlyphDesc(
		//	TVec2i(width, bitmap.rows),
		//	TVec2i(face->glyph->bitmap_left, face->glyph->bitmap_top),
		//	face->glyph->advance.x,
		//	bitmap_glyph->bitmap.buffer, i, true));
	}

	void* map = glyph_packer->BuildMap();
	glyph_packer->ClearImages();

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//if (r->Support.hw_generate_mipmap)
	//{
	//	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	//}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glyph_packer->GetTexSize(), glyph_packer->GetTexSize(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, map);

	glBindTexture(GL_TEXTURE_2D, 0);

	//SaveImage(glyph_packer.GetTexSize(), map);
	//auto any_error = ilGetError();

	CheckGLError();
	this->tex_font->fonts.emplace_back();
	this->tex_font->fonts.back().texture = tex_id;
	TTexFontId result;

	this->tex_font->fonts.back().glyphs.reset(glyph_packer);

	delete map;

	return result;
}

void SaveImage(int size, void* buffer)
{
	ilEnable(IL_FILE_OVERWRITE);
	GLuint image;
	ilGenImages(1, &image);
	ilBindImage(image);

	ilTexImage(size, size, 1, 2, IL_LUMINANCE_ALPHA, IL_UNSIGNED_BYTE, buffer);

	// zapisanie obrazu
	ilSave(IL_PNG, "fontmap.png");

	// wyłączenie parametru Mode
	ilDisable(IL_FILE_OVERWRITE);

	auto any_error = ilGetError();
}

void TBaluRender::TTexFont::Delete(TTexFontId use_font)
{
}

void DrawGlyph(TQuad<float, 3> &vertices, TQuad<float, 2> &tex_coord, TGlyph glyph, TGlyphDesc desc, float scale, TVec2 pos)
{

	float xpos = pos[0] + desc.Bearing[0] * scale;
	float ypos = pos[1] - (desc.Size[1] - desc.Bearing[1]) * scale;

	float w = desc.Size[0] * scale;
	float h = desc.Size[1] * scale;
	// Update VBO for each character

	vertices[0] = TVec3(xpos, ypos + h, 0);
	vertices[1] = TVec3(xpos, ypos, 0);
	vertices[2] = TVec3(xpos + w, ypos, 0);
	vertices[3] = TVec3(xpos + w, ypos + h, 0);

	tex_coord = glyph.tex_coords;
}

using namespace TBaluRenderEnums;

void TBaluRender::TTexFont::Print(TTexFontId use_font, TVec2 pos, char* text, ...)
{
	auto length = strlen(text);

	if (length > 0)
	{
		auto desc = &this->tex_font->fonts[use_font.id];
		glBindTexture(GL_TEXTURE_2D, desc->texture);

		auto& quads = this->tex_font->quads;
		auto& tex_coords = this->tex_font->tex_coords;

		if (quads.size() < length)
		{
			quads.resize(length);
			tex_coords.resize(length);
		}

		float curr_width = 0;
		for (int i = 0; i < length; i++)
		{
			auto glyph = desc->glyphs->GetGlyphByChar(text[i]);
			auto glyph_desc = desc->glyphs->GetDescById(glyph.glyph_desc_id);
			DrawGlyph(quads[i], tex_coords[i], glyph, glyph_desc, 1, TVec2(pos[0] + curr_width, pos[1]));
			curr_width += (glyph_desc.Advance >> 6);
		}

		TStreamsDesc streams;
		streams.AddStream(TStream::Vertex, TDataType::Float, 3, &quads[0]);

		streams.AddStream(TStream::TexCoord, 0, TDataType::Float, 2, &tex_coords[0]);

		this->r->Draw(streams, TPrimitive::Quads, 4 * length);

		streams.Clear();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}
