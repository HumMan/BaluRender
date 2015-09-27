#include "../../Include/baluRender.h"

using namespace BaluRender;

#include "../baluRenderCommon.h"

TBitmapFontId TBaluRender::TBitmapFont::Create()
{
	r->bitmap_fonts.emplace_back();

	unsigned int &base=r->bitmap_fonts.back().base;

	HFONT  font;           
	base = glGenLists(96);

	font = CreateFontA(  -24,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
		ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY,FF_DONTCARE|DEFAULT_PITCH,"Courier New");   
	SelectObject((HDC)r->p->hDC, font);
	wglUseFontBitmaps((HDC)r->p->hDC, 32, 96, base);
	TBitmapFontId result;
	result.id=r->bitmap_fonts.size()+1;
	return result;
}

void TBaluRender::TBitmapFont::Delete(TBitmapFontId use_font)
{
	glDeleteLists(r->bitmap_fonts[use_font.id].base, 96);
}

void TBaluRender::TBitmapFont::Print(TBitmapFontId use_font,TVec2 pos,char* format,...)
{
	char text[512];
	va_list ap;
	if(text==NULL)return;
	va_start(ap,format);
	int chars_to_print=vsprintf_s(text,format,ap);
	va_end(ap);
	glRasterPos2fv((GLfloat*)&pos);
	glListBase(r->bitmap_fonts[use_font.id].base-32);
	glCallLists(chars_to_print,GL_UNSIGNED_BYTE,text);
}

void TBaluRender::TBitmapFont::Print(TBitmapFontId use_font,TVec3 pos,char* format,...)
{
	char text[512];
	va_list ap;
	if(text==NULL)return;
	va_start(ap,format);
	int chars_to_print=vsprintf_s(text,format,ap);
	va_end(ap);
	glRasterPos3fv((GLfloat*)&pos);
	glListBase(r->bitmap_fonts[use_font.id].base-32);
	glCallLists(chars_to_print,GL_UNSIGNED_BYTE,text);

	
}