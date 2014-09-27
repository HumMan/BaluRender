#ifndef IMAGES_H
#define IMAGES_H

#include "baluLib.h"

#include <string>


enum class TFormat
{
	RGB8,
	RGBA8,
	LUMINANCE,
	RGB16F,
	DEPTH24,
	DXT1,
	DXT3,
	DXT5
};

class TImage
{
	char* pixels;
	int mip_maps_count;
	int width, height, depth;
	TFormat format;
public:
	TImage() :pixels(NULL){}
	~TImage()
	{
		if (!pixels)delete pixels;
	};
	void Load(const char* fname);
	void* GetPixels()
	{
		return pixels;
	};
	int GetWidth()
	{
		return width;
	};
	int GetHeight()
	{
		return height;
	};
	int GetDepth()
	{
		return depth;
	};
	int GetMipMapCount()
	{
		return mip_maps_count;
	};
	TFormat GetFormat()
	{
		return format;
	};
private:
	bool load_BMP(const char* fname);
	bool load_DDS(const char* fname);
	bool load_PNG(const char *fname);
};


#endif