#include "../images.h"

#include <fstream>

using namespace std;

#define MCHAR4(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))

#pragma pack (push, 1)
struct TDDS_PIXELFORMAT
{
        unsigned long dwSize;
        unsigned long dwFlags;
        unsigned long dwFourCC;
        unsigned long dwRGBBitCount;
        unsigned long dwRBitMask;
        unsigned long dwGBitMask;
        unsigned long dwBBitMask;
        unsigned long dwABitMask;
};

struct DDS_HEADER
{
        unsigned long dwSize;
        unsigned long dwFlags;
        unsigned long dwHeight;
        unsigned long dwWidth;
        unsigned long dwPitchOrLinearSize;
        unsigned long dwDepth;
        unsigned long dwMipMapCount;
        unsigned long dwReserved1[11];
        TDDS_PIXELFORMAT ddspf;
        unsigned long dwCaps1;
        unsigned long dwCaps2;
        unsigned long dwReserved2[3];
};

#pragma	pack (pop)

enum
{
  										// bit flags for header
	DDS_CAPS	    = 0x00000001,
	DDS_HEIGHT	    = 0x00000002,
	DDS_WIDTH	    = 0x00000004,
	DDS_PITCH	    = 0x00000008,
	DDS_PIXELFORMAT = 0x00001000,
	DDS_MIPMAPCOUNT = 0x00020000,
	DDS_LINEARSIZE  = 0x00080000,
	DDS_DEPTH	    = 0x00800000,

  										// flags for pixel formats
	DDS_ALPHA_PIXELS = 0x00000001,
	DDS_ALPHA        = 0x00000002,
	DDS_FOURCC	     = 0x00000004,
	DDS_RGB	         = 0x00000040,
    DDS_RGBA         = 0x00000041,

										// flags for complex caps
	DDS_COMPLEX	   = 0x00000008,
	DDS_TEXTURE	   = 0x00001000,
	DDS_MIPMAP	   = 0x00400000,

										// flags for cubemaps
	DDS_CUBEMAP	          = 0x00000200,
	DDS_CUBEMAP_POSITIVEX = 0x00000400,
	DDS_CUBEMAP_NEGATIVEX = 0x00000800,
	DDS_CUBEMAP_POSITIVEY = 0x00001000,
	DDS_CUBEMAP_NEGATIVEY = 0x00002000,
	DDS_CUBEMAP_POSITIVEZ = 0x00004000,
	DDS_CUBEMAP_NEGATIVEZ = 0x00008000,
	DDS_VOLUME		      = 0x00200000
};

static const int MAGIC_DDS    = MCHAR4( 'D', 'D', 'S', ' ' );
static const int FOURCC_DXT1  = MCHAR4( 'D', 'X', 'T', '1' );
static const int FOURCC_DXT3  = MCHAR4( 'D', 'X', 'T', '3' );
static const int FOURCC_DXT5  = MCHAR4( 'D', 'X', 'T', '5' );
static const int FOURCC_ATI1N = MCHAR4( 'A', 'T', 'I', '1' );
static const int FOURCC_ATI2N = MCHAR4( 'A', 'T', 'I', '2' );

bool TImage::load_DDS(const char* fname)
{
	DDS_HEADER		ddsd;
    char 			filecode [4];
    int 			factor;
    int 			bufferSize;
	FILE *file;

	if ((fopen_s(&file,fname, "rb")) !=0) 
		return false;
    									// Verify the file is a true .dds file
    fread(&filecode,4,1,file);

    if( strncmp( filecode, "DDS ", 4 ) != 0 )
        return false;

    									// Get the surface descriptor
    fread( &ddsd, sizeof ( ddsd ),1,file);

	if ( (ddsd.ddspf.dwFlags & DDS_FOURCC) == 0 )		// not compressed
	{
		int	numComponents = 0;

		if ( ddsd.ddspf.dwFlags & DDS_ALPHA_PIXELS )	// ARGB
			numComponents = 4;
		else											// RGB
			numComponents = 3;

		int	bytesPerLine = ddsd.dwWidth * numComponents;

		if ( (bytesPerLine & 3) != 0 )					// do dword alignment
			bytesPerLine += 4 - (bytesPerLine & 3);

		char    * buf     = new char [bytesPerLine];
		width = ddsd.dwWidth;
		height = ddsd.dwHeight;
		pixels = new char[width*height*numComponents];
		switch ( numComponents )
		{
			case 1:
				format = TFormat::LUMINANCE;
				break;

			case 3:
				format = TFormat::RGB8;
				break;

			case 4:
				format = TFormat::RGBA8;
				break;
		}
		for (unsigned int i = 0; i < ddsd.dwHeight; i++ )
		{
			fread( buf, bytesPerLine,1,file );

														// rearrange components
			char * dest = pixels + i * width * numComponents;
			char * src  = buf;

			for ( register unsigned int j = 0; j < ddsd.dwWidth; j++ )
			{
				dest [0] = src [2];						// red
				dest [1] = src [1];						// green
				dest [2] = src [0];						// blue

				if ( numComponents == 4 )
					dest [3] = src [3];					// alpha

				dest += numComponents;
				src  += numComponents;
			}
		}

		return true;
	}

    //
    // This .dds loader supports the loading of compressed formats DXT1, DXT3
    // and DXT5.
    //

    switch ( ddsd.ddspf.dwFourCC )
    {
        case FOURCC_DXT1:
            				// DXT1's compression ratio is 8:1
            format = TFormat::DXT1;
            factor = 2;
            break;

        case FOURCC_DXT3:
            				// DXT3's compression ratio is 4:1
            format = TFormat::DXT3;
            factor = 4;
            break;

        case FOURCC_DXT5:
            				// DXT5's compression ratio is 4:1
            format = TFormat::DXT5;
            factor = 4;
            break;

        default:
        	return false;
    }

    //
    // How big will the buffer need to be to load all of the pixel data
    // including mip-maps?
    //

    if( ddsd.dwPitchOrLinearSize == 0 )
    	return NULL;

	int	numComponents = 4;

    if( ddsd.ddspf.dwFourCC == FOURCC_DXT1 )
        numComponents = 3;

    if( ddsd.dwMipMapCount > 1 )
        bufferSize = ddsd.dwPitchOrLinearSize * factor;
    else
        bufferSize = ddsd.dwPitchOrLinearSize;

	mip_maps_count=ddsd.dwMipMapCount;
	width = ddsd.dwWidth;
	height = ddsd.dwHeight;
	pixels = new char[bufferSize];

    fread( pixels, bufferSize ,1, file);

	fclose(file);
	return true;
}

//TODO доделать загрузку dds с разжатием при необходимости
#if 0

int LoadDDSFile( const char* name, int& width, int& height, int& depth, int& bpp, char** data, int& tformat,
                int& numMipMaps,  bool decompress /*= false*/ )
{
    //if ( fs == NULL || name == NULL || data == NULL )
      //  return FALSE;

    //fEngine::IFile* file = fs->OpenFile( name, 0 );
	ifstream file(name);

    //if ( NULL == file )
      //  return FALSE;

    int ddsMagic;
    file.read( &ddsMagic ,sizeof( ddsMagic ));
    if ( MAGIC_DDS != ddsMagic )
    {
        file.close();
        return FALSE;
    }

    DDS_HEADER ddshdr;
    file.read( &ddshdr ,sizeof( ddshdr ));

    int numComponents;
    if ( ddshdr.ddspf.dwFlags & DDS_ALPHA_PIXELS )
        numComponents = 4;
    else
        numComponents = 3;

    bool is3DTexture = ( ( ddshdr.dwFlags & DDS_DEPTH ) != 0 ); // check for 3D texture

    width  = (int)ddshdr.dwWidth;
    height = (int)ddshdr.dwHeight;
    depth  = (int)ddshdr.dwDepth;
    bpp    = numComponents << 3;

    numMipMaps = (int)ddshdr.dwMipMapCount;
    //numMipMaps = 1;

    if ( !is3DTexture )
        depth = 1;

    if ( ( ddshdr.ddspf.dwFlags & DDS_FOURCC ) == 0 )   // not compressed
    {
        if ( 3 == numComponents )
            tformat = TEXTURE_FORMAT_RGB;
        else
            tformat = TEXTURE_FORMAT_RGBA;
        int ret = 0;
        if ( !is3DTexture )
            ret = DDS_LoadUncompressed2D( file, data, ddshdr, numComponents, numMipMaps );
        else
            ret = DDS_LoadUncompressed3D( file, data, ddshdr, numComponents, numMipMaps );
        fs->CloseFile( file );
        return ret;
    }

    bool bError = false;
    switch ( ddshdr.ddspf.dwFourCC )
    {
        case 34:            // RG16;
        case 36:            // RGBA16;
        case 111:           // R16F;
        case 112:           // RG16F;
        case 113:           // RGBA16F;
        case 114:           // R32F;
        case 115:           // RG32F;
        case 116:           // RGBA32F;
            bError = true; break;
        case FOURCC_DXT1:
        {
            if ( 3 == numComponents )
                tformat = TEXTURE_FORMAT_DXT1;
            else
                tformat = TEXTURE_FORMAT_DXT1A;
        } break;
        case FOURCC_DXT3:
            tformat = TEXTURE_FORMAT_DXT3;  break;
        case FOURCC_DXT5:
            tformat = TEXTURE_FORMAT_DXT5;  break;
        case FOURCC_ATI1N:
            tformat = TEXTURE_FORMAT_ATI1N; break;
        case FOURCC_ATI2N:
            tformat = TEXTURE_FORMAT_ATI2N; break;
        default:
            switch ( ddshdr.ddspf.dwRGBBitCount )
            {
                case 8:     // I8
                    bError = true; break;
            /*  case 16:
                {
                    //if      ( ddshdr.ddspf.dwRGBAlphaBitMask == 61440 ) // RGBA4
                    //else if ( ddshdr.ddspf.dwRGBAlphaBitMask == 65280 ) // IA8
                    //else if ( ddshdr.ddspf.dwBBitMask        == 31    ) // RGB565
                    //else // I16
                    bError = true;
                } break;
                case 24:
                    tformat = TEXTURE_FORMAT_RGB; break;
                case 32:
                    tformat = TEXTURE_FORMAT_RGBA; break;*/
                default:
                    bError = true;
            }
    }

    if ( bError )
    {
        fs->CloseFile( file );
        return false;
    }

    numComponents = fEngine::image::GetChannelCount( (_Texture_Format)tformat );
    bpp           = numComponents << 3;

    byte* pixbuf;
    int sizeToRead;
    if ( numMipMaps <= 1 )
    {
        sizeToRead = fEngine::image::GetImageSize( (_Texture_Format)tformat, width, height, depth, 1 );
        pixbuf = (byte*)my_malloc( sizeToRead );
        numMipMaps = 1;
    }
    else
    {
        int w = width;
        int h = height;
        sizeToRead = 0;
        for ( int i = 0; i < numMipMaps; ++i, w >>= 1, h >>= 1 )
        {
            if ( w < 1 ) w = 1;
            if ( h < 1 ) h = 1;
            sizeToRead += fEngine::image::GetImageSize( (_Texture_Format)tformat, w, h, depth, 1 );
        }
        pixbuf = (byte*)my_malloc( sizeToRead );
    }

    file->Read( sizeToRead, pixbuf );
    fs->CloseFile( file );

    if ( !decompress )
        *data = pixbuf;
    else
    {
        byte* uncompressed = (byte*)my_malloc( width * height * depth * numComponents );
        DDS_DecodeCompressedImage( uncompressed, pixbuf, width, height, tformat );
        *data = uncompressed;
        MY_SAFE_FREE( pixbuf );

        if ( tformat == TEXTURE_FORMAT_DXT1 )
            tformat = TEXTURE_FORMAT_RGB;
        else
            tformat = TEXTURE_FORMAT_RGBA;
    }

    return sizeToRead;
}


// internal functions

int DDS_LoadUncompressed2D( fEngine::IFile* file, byte** data, const DDS_HEADER& ddshdr, int numComponents, int& numMipMaps )
{
    int w            = (int)ddshdr.dwWidth;
    int h            = (int)ddshdr.dwHeight;
    int bytesPerLine = w * numComponents;

    if ( ( bytesPerLine & 3 ) != 0 )    // do dword alignment
        bytesPerLine += 4 - ( bytesPerLine & 3 );

    int fullSize = w * h * numComponents;
    byte* buf    = (byte*)my_malloc( bytesPerLine );
    byte* dest   = (byte*)my_malloc( fullSize );

    for ( register int i = 0; i < h; i++ )
    {
        file->Read( bytesPerLine, buf );

        byte* d = dest + ( i * w * numComponents );
        byte* s = buf;

        // rearrange components
        for ( register int j = 0; j < w; j++ )
        {
            d[ 0 ] = s[ 2 ];    // red
            d[ 1 ] = s[ 1 ];    // green
            d[ 2 ] = s[ 0 ];    // blue

            if ( 4 == numComponents )
                d[ 3 ] = s[ 3 ];    // alpha

            d += numComponents;
            s += numComponents;
        }
    }

    MY_SAFE_FREE( buf );
    *data = dest;

    numMipMaps = 1;

    // do mipmap adjustement
    //skipMipmaps ( data, w, h, bytesPerLine, numComponents, (int) ddsd.dwMipMapCount );

    return TRUE;
}

int DDS_LoadUncompressed3D( fEngine::IFile* file, byte** data, const DDS_HEADER& ddshdr, int numComponents, int& numMipMaps )
{
    int w            = (int)ddshdr.dwWidth;
    int h            = (int)ddshdr.dwHeight;
    int d            = (int)ddshdr.dwDepth;
    int bytesPerLine = w * numComponents;
    int rowsCount    = d * h;

    if ( ( bytesPerLine & 3 ) != 0 )    // do dword alignment
        bytesPerLine += 4 - ( bytesPerLine & 3 );

    int   fullSize = w * h * d * numComponents;
    byte* buf      = (byte*)my_malloc( bytesPerLine );
    byte* dest     = (byte*)my_malloc( fullSize );
    byte* dst      = dest;

    for ( register int i = 0; i < rowsCount; i++ )
    {
        file->Read( bytesPerLine, buf );

        byte* src  = buf;

        // rearrange components
        for ( register int j = 0; j < w; j++ )
        {
            dst[0] = src[2];             // red
            dst[1] = src[1];             // green
            dst[2] = src[0];             // blue

            if ( numComponents == 4 )
                dst[3] = src[3];         // alpha

            dst += numComponents;
            src += numComponents;
        }
    }

    MY_SAFE_FREE( buf );
    *data = dest;

    numMipMaps = 1;

    return TRUE;
}


/********************************************************************
*
*               =[ 0r@ngE ]= - software DDS decompress
*
*********************************************************************/

// Decodes a DXT color block
void DDS_DecodeColorBlock( byte* dest, const int w, const int h, const int xOff, const int yOff, const int format, byte* src )
{
    byte colors[4][3];

    word c0 = *(word*)src;
    word c1 = *(word*)(src + 2);

    // Extract the two stored colors
    colors[0][0] = ((c0 >> 11) & 0x1F) << 3;
    colors[0][1] = ((c0 >>  5) & 0x3F) << 2;
    colors[0][2] =  (c0        & 0x1F) << 3;

    colors[1][0] = ((c1 >> 11) & 0x1F) << 3;
    colors[1][1] = ((c1 >>  5) & 0x3F) << 2;
    colors[1][2] =  (c1        & 0x1F) << 3;

    register int i, x, y;

    // Compute the other two colors
    if ( c0 > c1 || TEXTURE_FORMAT_DXT5 == format )
    {
        for ( i = 0; i < 3; i++ )
        {
            colors[2][i] = ( 2 * colors[0][i] +     colors[1][i] + 1 ) / 3;
            colors[3][i] = (     colors[0][i] + 2 * colors[1][i] + 1 ) / 3;
        }
    }
    else
    {
        for ( i = 0; i < 3; i++ )
        {
            colors[2][i] = ( colors[0][i] + colors[1][i] + 1 ) >> 1;
            colors[3][i] = 0;
        }
    }

    src += 4;
    for ( y = 0; y < h; y++ )
    {
        byte* dst     = dest + yOff * y;
        dword indexes = src[ y ];
        for ( x = 0; x < w; x++ )
        {
            dword index = indexes & 0x3;
            dst[0]      = colors[index][0];
            dst[1]      = colors[index][1];
            dst[2]      = colors[index][2];
            indexes   >>= 2;

            dst += xOff;
        }
    }
}

// Decode a DXT3 alpha block
void DDS_DecodeDXT3AlphaBlock( byte* dest, const int w, const int h, const int xOff, const int yOff, byte* src )
{
    register int x, y;
    for ( y = 0; y < h; y++ )
    {
        byte* dst   = dest + yOff * y;
        dword alpha = ((dword*)src)[ y ];
        for ( x = 0; x < w; x++ )
        {
            *dst    = (alpha & 0xF) * 17;
            alpha >>= 4;
            dst    += xOff;
        }
    }
}

// Decode a DXT5 alpha block / 3Dc channel block
void DDS_DecodeDXT5AlphaBlock( byte* dest, const int w, const int h, const int xOff, const int yOff, byte *src )
{
    byte  a0    = src[0];
    byte  a1    = src[1];
    qword alpha = (*(qword*) src) >> 16;

    register int x, y;
    int k;

    for ( y = 0; y < h; y++ )
    {
        byte* dst = dest + yOff * y;
        for ( x = 0; x < w; x++ )
        {
            k = ((dword)alpha) & 0x7;
            if ( 0 == k )
                *dst = a0;
            else if ( 1 == k )
                *dst = a1;
            else if ( a0 > a1 )
                *dst = ((8 - k) * a0 + (k - 1) * a1) / 7;
            else if ( k >= 6 )
                *dst = (k == 6)? 0 : 255;
            else
                *dst = ((6 - k) * a0 + (k - 1) * a1) / 5;

            alpha >>= 3;
            dst    += xOff;
        }
        if ( w < 4 )
            alpha >>= (3 * (4 - w));
    }
}

// Decodes DXT and 3Dc formats
void DDS_DecodeCompressedImage( byte* dest, byte* src, const int width, const int height, const int format )
{
    int sx = ( width  < 4 ) ? width  : 4;
    int sy = ( height < 4 ) ? height : 4;

    int nChannels = fEngine::image::GetChannelCount( (_Texture_Format)format );
    for ( int y = 0; y < height; y += 4 )
    {
        for ( int x = 0; x < width; x += 4 )
        {
            byte *dst = dest + ( y * width + x ) * nChannels;
            if ( TEXTURE_FORMAT_DXT3 == format )
            {
                DDS_DecodeDXT3AlphaBlock( dst + 3, sx, sy, 4, width * 4, src );
                src += 8;
            }
            else if ( TEXTURE_FORMAT_DXT5 == format )
            {
                DDS_DecodeDXT5AlphaBlock( dst + 3, sx, sy, 4, width * 4, src );
                src += 8;
            }
            if ( format <= TEXTURE_FORMAT_DXT5 )
            {
                DDS_DecodeColorBlock( dst, sx, sy, nChannels, width * nChannels, format, src );
                src += 8;
            }
            else
            {
                if ( TEXTURE_FORMAT_ATI1N == format )
                {
                    DDS_DecodeDXT5AlphaBlock( dst, sx, sy, 1, width, src );
                    src += 8;
                }
                else
                {
                    DDS_DecodeDXT5AlphaBlock( dst,     sx, sy, 2, width * 2, src + 8 );
                    DDS_DecodeDXT5AlphaBlock( dst + 1, sx, sy, 2, width * 2, src );
                    src += 16;
                }
            }
        }
    }
}

#endif