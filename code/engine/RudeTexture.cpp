/*
 *  RudeTexture.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeTexture.h"
#include "RudeFile.h"
#include "RudeDebug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
#include <CoreFoundation/CFBundle.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGBitmapContext.h>
#endif

#if defined(RUDE_WIN)
#include <FreeImage.h>
#endif

#if defined(RUDE_AMIGAOS4)
#include <png.h>
#endif

#include "PVRTTexture.h"
#include "PVRTTextureAPI.h"

RudeTexture::RudeTexture()
: m_height(0)
, m_width(0)
, m_texture(-1)
{
	m_name[0] = '\0';
}

RudeTexture::~RudeTexture()
{
	if(m_texture != static_cast<unsigned int>(-1))
	{
		glDeleteTextures(1, &m_texture);
		m_texture = -1;
	}
}

int RudeTexture::LoadFromPVRTFile(const char *name)
{
	if(name == NULL || name[0] == '\0')
		return -1;
	RUDE_REPORT("LoadFromPVRTFile %s\n", name);
	
	// First, check if an uncompressed/high-res .png exists for this texture
	char pngbasename[kNameLen];
	strncpy(pngbasename, name, kNameLen - 1);
	pngbasename[kNameLen - 1] = '\0';
	char *ext = strrchr(pngbasename, '.');
	if(ext != NULL && (strcasecmp(ext, ".pvr") == 0 || strcasecmp(ext, ".bmp") == 0 || strcasecmp(ext, ".tga") == 0 || strcasecmp(ext, ".png") == 0))
		*ext = '\0';

	char pngfilename[kNameLen + 4];
	snprintf(pngfilename, sizeof(pngfilename), "%s.png", pngbasename);

	char pngfilepath[512];
	if(RudeFileGetFile(pngfilename, pngfilepath, sizeof(pngfilepath), true))
	{
		bool isTerrain = (strncasecmp(pngbasename, "grass_", 6) == 0 ||
		                  strncasecmp(pngbasename, "dirt_", 5) == 0 ||
		                  strncasecmp(pngbasename, "sand_", 5) == 0);
		int res = LoadFromPNG(pngbasename, isTerrain);
		if(res == 0)
		{
			strncpy(m_name, name, kNameLen - 1);
			m_name[kNameLen - 1] = '\0';
			return 0;
		}
	}

	strncpy(m_name, name, kNameLen - 1);
	m_name[kNameLen - 1] = '\0';
	
	char filename[kNameLen + 4];
	const char *extension = strrchr(name, '.');
	if(extension != NULL && strcasecmp(extension, ".pvr") == 0)
		snprintf(filename, sizeof(filename), "%s", name);
	else
		snprintf(filename, sizeof(filename), "%s.pvr", name);
	
	char filepath[512];
	if(!RudeFileGetFile(filename, filepath, sizeof(filepath), false))
		return -1;
	
	PVR_Texture_Header header;

	unsigned int result = PVRTLoadTextureFromPVR(filepath, &m_texture, &header);
	
	if(result == 0)
		return -1;
	
	m_height = header.dwHeight;
	m_width = header.dwWidth;
	
	return 0;

}

int RudeTexture::LoadFromPVRTPointer(const char *name, const void *data)
{
	if(name == NULL || data == NULL)
		return -1;
	strncpy(m_name, name, kNameLen - 1);
	m_name[kNameLen - 1] = '\0';
	
	PVR_Texture_Header header;
	
	int result = PVRTLoadTextureFromPointer(data, &m_texture, &header);
	
	if(result == 0)
		return -1;
	
	m_height = header.dwHeight;
	m_width = header.dwWidth;
	
	return 0;
}

int RudeTexture::LoadFromPNG(const char *name, bool genMipmaps)
{	
	if(name == NULL || name[0] == '\0')
		return -1;
	// flush glGetError
	glGetError();

	strncpy(m_name, name, kNameLen - 1);
	m_name[kNameLen - 1] = '\0';

	char filename[128];
	const char *extension = strrchr(name, '.');
	if(extension != NULL && strcasecmp(extension, ".png") == 0)
		snprintf(filename, sizeof(filename), "%s", name);
	else
		snprintf(filename, sizeof(filename), "%s.png", name);

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)

	int result = -1;
	
	CFStringRef cfFilename = CFStringCreateWithCString(0, filename, kCFStringEncodingASCII);
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourceURL(mainBundle, cfFilename, 0, 0);
	
	CGDataProviderRef ref;
	CGImageRef image;
	int error = 0;
	GLubyte *imageData = 0;
	CGContextRef imageContext;
	
	if(url == NULL)
		goto LoadFromPNG_URLFail;
	
	ref = CGDataProviderCreateWithURL(url);
	if(ref == NULL)
		goto LoadFromPNG_URLProviderFail;
	
	image = CGImageCreateWithPNGDataProvider(ref, 0, false, kCGRenderingIntentDefault);
	if(image == NULL)
		goto LoadFromPNG_ImageRefFail;
	
	CGDataProviderRelease(ref);
	
	m_width = CGImageGetWidth(image);
	m_height = CGImageGetHeight(image);
	
	imageData = (GLubyte *) malloc(m_width * m_height * 4);
	RUDE_ASSERT(imageData, "Failed to allocate space for texture storage");
	
	memset(imageData, 0, m_width * m_height * 4);
	
	imageContext = CGBitmapContextCreate(imageData, m_width, m_height, 8, m_width * 4, CGImageGetColorSpace(image), kCGImageAlphaPremultipliedLast);
	CGContextDrawImage(imageContext, CGRectMake(0.0, 0.0, (CGFloat) m_width, (CGFloat) m_height), image);
	CGContextRelease(imageContext);
	
	
	glGenTextures(1, &m_texture);
	RUDE_ASSERT(m_texture >= 0, "Failed to gen texture");
	
	glBindTexture(GL_TEXTURE_2D, m_texture);
	
	error = glGetError();
	RUDE_ASSERT(error == 0, "glBindTexture failed on texture id %d (%s), error=%x", m_texture, name, error);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	free(imageData);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	result = 0;
	
	CFRelease(image);
	
LoadFromPNG_ImageRefFail:
	
	
LoadFromPNG_URLProviderFail:
	CFRelease(url);
	
LoadFromPNG_URLFail:
	CFRelease(cfFilename);
	
	return result;
#endif // defined(RUDE_IPHONE) || defined(RUDE_MACOS)

#if defined(RUDE_WIN)

	// Find where the file is located

	char buffer[256];
	bool found = RudeFileGetFile(filename, buffer, 256, false);
	RUDE_ASSERT(found, "Texture not found: %s", filename);

	// Retrieve file attributes and load the file

	FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(buffer, 0);
	FIBITMAP* imagen = FreeImage_Load(formato, buffer);

	FIBITMAP* temp = imagen;
	imagen = FreeImage_ConvertTo32Bits(imagen);
	FreeImage_Unload(temp);

	m_width = FreeImage_GetWidth(imagen);
	m_height = FreeImage_GetHeight(imagen);
	RUDE_REPORT("LoadFromPNG %s (%dx%d)\n", filename, m_width, m_height);

	// Endian-swap and vertically flip the loaded texture

	GLubyte * textura = (GLubyte *) malloc(4 * m_width * m_height);
	RUDE_ASSERT(textura, "malloc failed");

	char* pixeles = (char*) FreeImage_GetBits(imagen);

	for(int y = 0; y < m_height; y++)
	{
		for(int x = 0; x < m_width; x++)
		{
			int s = y * m_width + x;
			int d = (m_height-y-1) * m_width + x;
			textura[d*4+0]= pixeles[s*4+2];
			textura[d*4+1]= pixeles[s*4+1];
			textura[d*4+2]= pixeles[s*4+0];
			textura[d*4+3]= pixeles[s*4+3];
		}
	}

	// Generate the OpenGL texture object 

	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA, m_width, m_height, 0, GL_RGBA,GL_UNSIGNED_BYTE, (GLvoid*)textura);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLenum loaderror = glGetError();
	RUDE_ASSERT(loaderror == 0, "Failed to load texture %d", loaderror);

	free(textura);
	FreeImage_Unload(imagen);

	return 0;
#endif

#if defined(RUDE_AMIGAOS4)
	char filepath[512];
	if(!RudeFileGetFile(filename, filepath, sizeof(filepath), true))
		return -1;

	RUDE_REPORT("LoadFromPNG %s (%s)\n", filename, filepath);

	FILE *file = fopen(filepath, "rb");
	if(file == NULL)
		return -1;

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		NULL, NULL);
	png_infop info = png == NULL ? NULL : png_create_info_struct(png);
	if(png == NULL || info == NULL)
	{
		if(info != NULL)
			png_destroy_read_struct(&png, NULL, NULL);
		else if(png != NULL)
			png_destroy_read_struct(&png, NULL, NULL);
		fclose(file);
		return -1;
	}

	if(setjmp(png_jmpbuf(png)) != 0)
	{
		png_destroy_read_struct(&png, &info, NULL);
		fclose(file);
		return -1;
	}

	png_init_io(png, file);
	png_read_info(png, info);

	png_uint_32 width = png_get_image_width(png, info);
	png_uint_32 height = png_get_image_height(png, info);
	int colorType = png_get_color_type(png, info);
	int bitDepth = png_get_bit_depth(png, info);

	if(bitDepth == 16)
		png_set_strip_16(png);
	if(colorType == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);
	if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
		png_set_expand_gray_1_2_4_to_8(png);
	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);
	if(colorType == PNG_COLOR_TYPE_GRAY ||
	   colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);
	if((colorType & PNG_COLOR_MASK_ALPHA) == 0 &&
	   !png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);

	png_read_update_info(png, info);
	png_size_t rowBytes = png_get_rowbytes(png, info);
	unsigned char *pixels = static_cast<unsigned char *>(
		malloc(rowBytes * height));
	png_bytep *rows = static_cast<png_bytep *>(
		malloc(sizeof(png_bytep) * height));
	if(pixels == NULL || rows == NULL)
	{
		free(rows);
		free(pixels);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(file);
		return -1;
	}

	for(png_uint_32 y = 0; y < height; ++y)
		rows[y] = pixels + y * rowBytes;
	png_read_image(png, rows);
	png_read_end(png, info);
	fclose(file);
	png_destroy_read_struct(&png, &info, NULL);
	free(rows);

	if(rowBytes != width * 4)
	{
		free(pixels);
		return -1;
	}

	m_width = static_cast<int>(width);
	m_height = static_cast<int>(height);
	while(glGetError() != GL_NO_ERROR);
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	if(genMipmaps)
	{
		int curW = m_width;
		int curH = m_height;
		int level = 0;
		unsigned char *curPixels = pixels;
		unsigned char *prevBuf = NULL;

		while(curW > 1 || curH > 1)
		{
			int nextW = curW > 1 ? curW / 2 : 1;
			int nextH = curH > 1 ? curH / 2 : 1;
			unsigned char *nextBuf = (unsigned char *) malloc(nextW * nextH * 4);
			if(!nextBuf) break;

			for(int y = 0; y < nextH; y++)
			{
				for(int x = 0; x < nextW; x++)
				{
					int sx = x * 2;
					int sy = y * 2;
					int sx2 = (curW > 1) ? (sx + 1) : sx;
					int sy2 = (curH > 1) ? (sy + 1) : sy;

					for(int c = 0; c < 4; c++)
					{
						int p00 = curPixels[(sy * curW + sx) * 4 + c];
						int p01 = curPixels[(sy * curW + sx2) * 4 + c];
						int p10 = curPixels[(sy2 * curW + sx) * 4 + c];
						int p11 = curPixels[(sy2 * curW + sx2) * 4 + c];
						nextBuf[(y * nextW + x) * 4 + c] = (unsigned char)((p00 + p01 + p10 + p11) / 4);
					}
				}
			}
			level++;
			glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, nextW, nextH, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, nextBuf);

			if(prevBuf) free(prevBuf);
			prevBuf = nextBuf;
			curPixels = nextBuf;
			curW = nextW;
			curH = nextH;
		}
		if(prevBuf) free(prevBuf);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		static bool s_anisoChecked = false;
		static float s_maxAniso = 1.0f;
		if(!s_anisoChecked)
		{
			s_anisoChecked = true;
			const char *exts = (const char *)glGetString(GL_EXTENSIONS);
			if(exts && strstr(exts, "GL_EXT_texture_filter_anisotropic"))
			{
				glGetFloatv(0x84FF /* GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT */, &s_maxAniso);
				if(s_maxAniso > 8.0f) s_maxAniso = 8.0f;
			}
		}
		if(s_maxAniso > 1.0f)
		{
			glTexParameterf(GL_TEXTURE_2D, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY_EXT */, s_maxAniso);
		}
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	free(pixels);

	GLenum err = glGetError();
	if(err != GL_NO_ERROR)
	{
		RUDE_REPORT("LoadFromPNG notice: glGetError()=0x%x for %s\n", err, filename);
	}
	return 0;
#endif
}



void RudeTexture::SetActive()
{
	
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glEnable(GL_TEXTURE_2D);
	//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	
	
}
