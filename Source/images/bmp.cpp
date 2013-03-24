#include "../images.h"

#define uint32 unsigned int
#define uint16 unsigned short
#define uint8 unsigned char

#define MCHAR2(a, b) (a | (b << 8))
#define MCHAR4(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

template <typename DATA_TYPE>
inline void swapChannels(DATA_TYPE *pixels, int nPixels, const int channels, const int ch0, const int ch1){
	do {
		DATA_TYPE tmp = pixels[ch1];
		pixels[ch1] = pixels[ch0];
		pixels[ch0] = tmp;
		pixels += channels;
	} while (--nPixels);
}

#pragma pack (push, 1)
struct BMPHeader {
	uint16 bmpIdentifier;
	uint8  junk[16];
	uint32 width;
	uint32 height;
	uint16 junk2;
	uint16 bpp;
	uint16 compression;
	uint8  junk3[22];
};
#pragma pack (pop)

bool TImage::load_BMP(const char* fname)
{
	BMPHeader header;

	FILE *file;
	char *dest;
	int i;

	if ((fopen_s(&file,fname, "rb")) !=0) 
		return false;
	
	// Read the header
	fread(&header, sizeof(header), 1, file);
	if (header.bmpIdentifier != MCHAR2('B', 'M')){
		fclose(file);
		return false;
	}

	width  = header.width;
	height = header.height;
	depth  = 1;
	mip_maps_count = 1;

	switch (header.bpp){
	case 24:
	case 32:
		int nChannels;
		nChannels = (header.bpp == 24)? 3 : 4;
		format    = (header.bpp == 24)? TFormat::RGB8 : TFormat::RGBA8;
		pixels = new char[width * height * nChannels];
		for (i = height - 1; i >= 0; i--){
			dest = pixels + i * width * nChannels;
			fread(dest, width * nChannels, 1, file);
			swapChannels(dest, width, nChannels, 0, 2);
		}
		break;
	default:
		fclose(file);
		return false;
	}

	fclose(file);

	return true;
};

