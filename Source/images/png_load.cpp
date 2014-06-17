
#include "../images.h"
#include "png.h"

png_voidp malloc_fn(png_structp png_ptr, png_size_t size){
	return malloc(size);
}
void free_fn(png_structp png_ptr, png_voidp ptr){
	free(ptr);
}

void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length){
	fwrite(data, length, 1, (FILE *) png_get_io_ptr(png_ptr));
}

void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length){
	fread(data, length, 1, (FILE *) png_get_io_ptr(png_ptr));
}

bool TImage::load_PNG(const char *fname){
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
    FILE *file;

    // open the PNG input file
    if ((fopen_s(&file,fname, "rb")) !=NULL) return false;

    // first check the eight byte PNG signature
    png_byte pbSig[8];
    fread(pbSig, 1, 8, file);
    if (!png_check_sig(pbSig, 8)){
		fclose(file);
		return false;
	}

    // create the two png(-info) structures
    if ((png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, malloc_fn, free_fn)) == NULL){
		fclose(file);
        return false;
    }

    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(file);
        return false;
    }

	// initialize the png structure
	png_set_read_fn(png_ptr, file, user_read_data);
	png_set_sig_bytes(png_ptr, 8);
	
	// read all PNG info up to image data
	png_read_info(png_ptr, info_ptr);

	// get width, height, bit-depth and color-type
	png_uint_32 w, h;
    int bitDepth, colorType;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bitDepth, &colorType, NULL, NULL, NULL);

	width = w;
	height = h;
	depth = 1;
	mip_maps_count = 1;
	int arraySize = 1;
	assert(bitDepth ==8);
	int nChannels = png_get_channels(png_ptr, info_ptr);
	switch (nChannels){
		case 3:
			format = TFormat::RGB8;
			break;
		case 4:
			format = TFormat::RGBA8;
			break;
		default: // and we're done
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(file);
			return false;
	}

	int rowSize = width * nChannels * bitDepth / 8;

	// now we can allocate memory to store the image
	pixels = new char[rowSize * height];
	
	// set the individual row-pointers to point at the correct offsets
    png_byte **ppbRowPointers = new png_bytep[height];
	for (int i = 0; i < height; i++)
		ppbRowPointers[i] = (unsigned char*)(pixels + i * rowSize);

	// now we can go ahead and just read the whole image
	png_read_image(png_ptr, ppbRowPointers);

	// read the additional chunks in the PNG file (not really needed)
	png_read_end(png_ptr, NULL);
	
	delete [] ppbRowPointers;

	if (colorType == PNG_COLOR_TYPE_PALETTE){
		png_colorp palette=NULL;
		int num_palette;
		png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);

		unsigned char *newPixels = new unsigned char[width * height * 3];
		if (bitDepth == 4){
			for (int i = 0; i < rowSize * height; i++){
				unsigned int i0 = pixels[i] >> 4;
				unsigned int i1 = pixels[i] & 0xF;
				newPixels[6 * i    ] = palette[i0].red;
				newPixels[6 * i + 1] = palette[i0].green;
				newPixels[6 * i + 2] = palette[i0].blue;
				newPixels[6 * i + 3] = palette[i1].red;
				newPixels[6 * i + 4] = palette[i1].green;
				newPixels[6 * i + 5] = palette[i1].blue;
			}
		} else {
			for (int i = 0; i < rowSize * height; i++){
				newPixels[3 * i    ] = palette[pixels[i]].red;
				newPixels[3 * i + 1] = palette[pixels[i]].green;
				newPixels[3 * i + 2] = palette[pixels[i]].blue;
			}
		}
		format = TFormat::RGB8;

		delete [] pixels;
		pixels = (char*)newPixels;
	}

	if (bitDepth == 16){
		// Fix endian
		int size = width * height * nChannels * sizeof(unsigned short);
		for (int i = 0; i < size; i += 2){
			unsigned char tmp = pixels[i];
			pixels[i] = pixels[i + 1];
			pixels[i + 1] = tmp;
		}
	}

	// and we're done
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    fclose(file);

    return true;
}