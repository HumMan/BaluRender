#include "../../Include/baluRender.h"

using namespace BaluRender;

#include "../baluRenderCommon.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H
#include FT_CACHE_CHARMAP_H

const int tex_size=1024*1;
const int glyph_size=96;

struct TGlyphPacker
{
	struct TGlyph
	{
		TVec2i pos,size;
		TVec2 tex_coords[4]; // обход прямоугольника CCW
		TGlyph(TVec2i use_pos,TVec2i use_size):pos(use_pos),size(use_size)
		{
			TVec2i t;//TODO походу надо зеркалировать
			t=(pos+size.ComponentMul(TVec2i(0,0)))*(1.0/tex_size);
			tex_coords[0]=TVec2(t[0],t[1]);
			t=(pos+size.ComponentMul(TVec2i(0,1)))*(1.0/tex_size);
			tex_coords[1]=TVec2(t[0],t[1]);
			t=(pos+size.ComponentMul(TVec2i(1,1)))*(1.0/tex_size);
			tex_coords[2]=TVec2(t[0],t[1]);
			t=(pos+size.ComponentMul(TVec2i(1,0)))*(1.0/tex_size);
			tex_coords[3]=TVec2(t[0],t[1]);
		}
		TGlyph(){}
	};
	struct TRow
	{
		std::vector<TGlyph> glyph;
		int curr_width;
		TRow():curr_width(0){}
		TVec2i PushGlyph(int curr_heigth,TVec2i size)
		{
			curr_width+=size[0]+1;
			glyph.push_back(TGlyph(TVec2i(curr_width,curr_heigth),size));
			return glyph.back().pos;
		}
	};
	int max_heigh;
	int curr_height;
	std::vector<TRow> rows;
	TGlyphPacker():max_heigh(0),curr_height(0){}
	TVec2i PushGlyph(TVec2i size)
	{
		if(rows.size()==0||rows.back().curr_width+size[0]>tex_size)
		{
			if(rows.size()!=0)
			{
				curr_height+=max_heigh+1;
				max_heigh=0;
			}
			rows.emplace_back();
		}
		if(size[1]>max_heigh)max_heigh=size[1];
		return rows.back().PushGlyph(curr_height,size);

	}
}glyph_packer;

#if PLATFORM==PLATFORM_WIN32
#  define FREETYPE_PLATFORM TT_PLATFORM_MICROSOFT
#  define FREETYPE_ENCODING TT_MS_ID_UNICODE_CS
#elif PLATFORM==PLATFORM_LINUX // не проверено - в документации по этому поводу как-то коряво написано
#  define FREETYPE_PLATFORM TT_PLATFORM_APPLE_UNICODE
#  define FREETYPE_ENCODING TT_APPLE_ID_DEFAULT
#endif

TTexFontId TBaluRender::TTexFont::Create()
{
	int h=60;
	FT_Error error;
	FT_Library library;
	error = FT_Init_FreeType( &library );
	FT_Face face;
	//error = FT_New_Face( library, "Vera.ttf", 0, &face );
	//error = FT_New_Face( library, "wt021.ttf", 0, &face );
	error = FT_New_Face( library, "times.ttf", 0, &face );

	error = FT_Set_Char_Size(
            face,    /* handle to face object           */
            0,       /* char_width in 1/64th of points  */
            16*64,   /* char_height in 1/64th of points */
            300,     /* horizontal device resolution    */
            300 );   /* vertical device resolution      */
	error = FT_Set_Pixel_Sizes(
            face,   /* handle to face object */
            0,      /* pixel_width           */
            80 );   /* pixel_height          */

	{
		FT_CharMap  found = 0;
		FT_CharMap  charmap;
		int         n;
		for ( n = 0; n < face->num_charmaps; n++ )
		{
			charmap = face->charmaps[n];
			if ( charmap->platform_id == FREETYPE_PLATFORM &&
				charmap->encoding_id == FREETYPE_ENCODING  )
			{
				found = charmap;
				break;
			}
		}
		error = FT_Set_Charmap( face, found );
	}

	unsigned char* buff = new unsigned char[sqr(tex_size)*2];
	memset(buff,0,sqr(tex_size)*2);

	char text[]="_abcdefghigklmnopqrstuvwxyzABCDEFGHIGKLMNOPQRSTUVWXYZ";
	int len=strlen(text);

	for(int i=0;i<len;i++)
	{
		FT_UInt glyph_index = FT_Get_Char_Index(face, text[i]);
		if(glyph_index==0)continue;
		error = FT_Load_Glyph(face,glyph_index,FT_LOAD_DEFAULT);

		FT_Glyph glyph;
		FT_Get_Glyph(face->glyph,&glyph);
		error = FT_Glyph_To_Bitmap(&glyph,ft_render_mode_normal,0,1);
		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
		FT_Bitmap &bitmap=bitmap_glyph->bitmap;
		int width=bitmap.width;
		int height=bitmap.rows;
		
		TVec2i glyph_tex_pos=glyph_packer.PushGlyph(TVec2i(width,height));
		for(int i=0;i<width;i++)
			for(int k=0;k<height;k++)
			{
				int p=(i+glyph_tex_pos[0])+tex_size*(k+glyph_tex_pos[1]);
				buff[p*2+0]=bitmap.buffer[i+width*k];
				buff[p*2+1]=bitmap.buffer[i+width*k];
			}
		delete glyph;
	}

	GLuint tex_id;
	glGenTextures(1,&tex_id);
	glBindTexture(GL_TEXTURE_2D,tex_id);
	if(r->Support.hw_generate_mipmap)
	{
		glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
	}
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tex_size,tex_size,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,buff);
	glBindTexture(GL_TEXTURE_2D,0);

	CheckGLError();
	r->tex_fonts.emplace_back();
	r->tex_fonts.back().tex=tex_id;
	TTexFontId result;

	delete buff;

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return result;
}

void TBaluRender::TTexFont::Delete(TTexFontId use_font)
{
}
void TBaluRender::TTexFont::Print(TTexFontId use_font,char* text,...)
{
	glBindTexture(GL_TEXTURE_2D,r->tex_fonts[use_font.id].tex);
}
