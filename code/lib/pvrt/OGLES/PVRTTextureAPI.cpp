/******************************************************************************

 @File         PVRTTextureAPI.cpp

 @Title        

 @Copyright    Copyright (C) 2004 - 2008 by Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  GL texture loading.

******************************************************************************/
#include <string.h>
#include <stdlib.h>

#include "PVRTContext.h"
#include "PVRTglesExt.h"
#include "PVRTTexture.h"
#include "PVRTTextureAPI.h"
#include "PVRTDecompress.h"
#include "PVRTFixedPoint.h"
#include "PVRTMatrix.h"
#include "PVRTMisc.h"
#include "PVRTResourceFile.h"

#if defined(RUDE_AMIGAOS4)
extern "C" void glCompressedTexImage2D(GLenum target, GLint level,
	GLenum internalformat, GLsizei width, GLsizei height, GLint border,
	GLsizei imageSize, const GLvoid *data);
#endif

/*****************************************************************************
** Functions
****************************************************************************/


/*!***************************************************************************
@Function		PVRTTextureTile
@Modified		pOut		The tiled texture in system memory
@Input			pIn			The source texture
@Input			nRepeatCnt	Number of times to repeat the source texture
@Description	Allocates and fills, in system memory, a texture large enough
				to repeat the source texture specified number of times.
*****************************************************************************/
void PVRTTextureTile(
	PVR_Texture_Header			**pOut,
	const PVR_Texture_Header	* const pIn,
	const int					nRepeatCnt)
{
	unsigned int		nFormat = 0, nType = 0, nBPP, nSize, nElW = 0, nElH = 0;
	unsigned char		*pMmSrc, *pMmDst;
	unsigned int		nLevel;
	PVR_Texture_Header	*psTexHeaderNew;

	_ASSERT(pIn->dwWidth);
	_ASSERT(pIn->dwWidth == pIn->dwHeight);
	_ASSERT(nRepeatCnt > 1);

	switch(pIn->dwpfFlags & PVRTEX_PIXELTYPE)
	{
	case OGL_RGBA_5551:
		nFormat		= GL_UNSIGNED_SHORT_5_5_5_1;
		nType		= GL_RGBA;
		nElW		= 1;
		nElH		= 1;
		break;
	case OGL_RGBA_8888:
		nFormat		= GL_UNSIGNED_BYTE;
		nType		= GL_RGBA;
		nElW		= 1;
		nElH		= 1;
		break;
	case OGL_PVRTC2:
		nFormat		= GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
		nType		= 0;
		nElW		= 8;
		nElH		= 4;
		break;
	case OGL_PVRTC4:
		nFormat		= GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
		nType		= 0;
		nElW		= 4;
		nElH		= 4;
		break;
	}

	nBPP = PVRTTextureFormatBPP(nFormat, nType);
	nSize = pIn->dwWidth * nRepeatCnt;

	psTexHeaderNew	= PVRTTextureCreate(nSize, nSize, nElW, nElH, nBPP, true);
	*psTexHeaderNew	= *pIn;
	pMmDst	= (unsigned char*)psTexHeaderNew + sizeof(*psTexHeaderNew);
	pMmSrc	= (unsigned char*)pIn + sizeof(*pIn);

	for(nLevel = 0; ((unsigned int)1 << nLevel) < nSize; ++nLevel)
	{
		int nBlocksDstW = PVRT_MAX((unsigned int)1, (nSize >> nLevel) / nElW);
		int nBlocksDstH = PVRT_MAX((unsigned int)1, (nSize >> nLevel) / nElH);
		int nBlocksSrcW = PVRT_MAX((unsigned int)1, (pIn->dwWidth >> nLevel) / nElW);
		int nBlocksSrcH = PVRT_MAX((unsigned int)1, (pIn->dwHeight >> nLevel) / nElH);
		int nBlocksS	= nBPP * nElW * nElH / 8;

		PVRTTextureLoadTiled(
			pMmDst,
			nBlocksDstW,
			nBlocksDstH,
			pMmSrc,
			nBlocksSrcW,
			nBlocksSrcH,
			nBlocksS,
			(pIn->dwpfFlags & PVRTEX_TWIDDLE) ? true : false);

		pMmDst += nBlocksDstW * nBlocksDstH * nBlocksS;
		pMmSrc += nBlocksSrcW * nBlocksSrcH * nBlocksS;
	}

	psTexHeaderNew->dwWidth = nSize;
	psTexHeaderNew->dwHeight = nSize;
	psTexHeaderNew->dwMipMapCount = nLevel;
	*pOut = psTexHeaderNew;
}

/*!***************************************************************************
 @Function		PVRTLoadDecompressedTextureFromPointer
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Input			pointer			Pointer to the header-prefixed texture from
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in header files and loaded in. Loads the whole texture
				Release texture by calling PVRTReleaseTexture.  Decompresses to RGBA8888 internally.
*****************************************************************************/
unsigned int PVRTLoadDecompressedTextureFromPointer(const void* pointer, GLuint *texName, const void *psTextureHeader)
{
	return PVRTLoadDecompressedPartialTextureFromPointer(pointer, 0, texName, psTextureHeader);
}

/*!***************************************************************************
 @Function		PVRTLoadDecompressedPartialTextureFromPointer
 @Input			pointer			Pointer to the header-prefixed texture from
 @Input			nLoadFromLevel	Which mipmap level to start loading from (0=all)
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in header files and loaded in. Loads the whole texture
				Release texture by calling PVRTReleaseTexture.  Decompresses to RGBA8888 internally.
*****************************************************************************/
unsigned int PVRTLoadDecompressedPartialTextureFromPointer(const void *pointer,
														   unsigned int nLoadFromLevel,
														   GLuint *texName,
														   const void *psTextureHeader)
{
	//Malloc off a duplicate pointer for the header _only_
	PVR_Texture_Header *oldHeader = (PVR_Texture_Header*)pointer;

	if(((oldHeader->dwpfFlags&PVRTEX_PIXELTYPE) != OGL_PVRTC2) && ((oldHeader->dwpfFlags&PVRTEX_PIXELTYPE) != OGL_PVRTC4))
	{
		return PVRTLoadPartialTextureFromPointer(pointer, 0, 0, texName, psTextureHeader);
	}

	PVR_Texture_Header *newHeader = (PVR_Texture_Header*)malloc(sizeof(PVR_Texture_Header));
	memcpy(newHeader,pointer,sizeof(PVR_Texture_Header));

	//Change the decompressed texture header's format to be RGBA8888, drop top mip level
	newHeader->dwpfFlags = OGL_RGBA_8888;
	newHeader->dwpfFlags |= (oldHeader->dwpfFlags & PVRTEX_MIPMAP);
	newHeader->dwMipMapCount--;
	newHeader->dwBitCount = 32;
	newHeader->dwWidth /= 2;
	newHeader->dwHeight /= 2;
	int newSize = 0;

	//Malloc width*height*miplevels*4 bytes
	int szx = newHeader->dwWidth;
	int szy = newHeader->dwHeight;
	int nMips = 1;
	while(szx && szy){
		newSize += 4*szx*szy;
		szx>>=1;
		szy>>=1;
		nMips++;
	}

	unsigned char *newTexture = (unsigned char*)malloc(newSize);

	//Decompress each texture layer into the new memory
	szx = newHeader->dwWidth;
	szy = newHeader->dwHeight;
	unsigned char *thisMipLevelSrc = (unsigned char*)pointer+sizeof(PVR_Texture_Header)+
		(oldHeader->dwWidth*oldHeader->dwHeight*oldHeader->dwBitCount + 7)/8;
	unsigned char *thisMipLevelDest = newTexture;
	while(szx && szy){
		PVRTCDecompress(thisMipLevelSrc,
				((oldHeader->dwpfFlags&PVRTEX_PIXELTYPE) == OGL_PVRTC2) ? 1 : 0,
				szx,
				szy,
				thisMipLevelDest);
		thisMipLevelSrc += (szx * szy * oldHeader->dwBitCount + 7)/8;
		thisMipLevelDest += (szx * szy * 4);
		szx>>=1;
		szy>>=1;
	}

	//Load the textures
	unsigned int result = PVRTLoadPartialTextureFromPointer(newHeader,newTexture, 0, texName, psTextureHeader);

	// make sure new header is passed back rather than file's header
	*(PVR_Texture_Header*)(psTextureHeader) = *newHeader;

	//Bin the temporary header and memory
	FREE(newHeader);
	FREE(newTexture);

	return result;
}

/*!***************************************************************************
 @Function		PVRTLoadPartialTextureFromPointer
 @Input			pointer			Pointer to header-texture's structure
 @Input			texPtr			If null, texture follows header, else texture is here.
 @Input			nLoadFromLevel	Which mipmap level to start loading from (0=all)
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		flags from texture on success, 0 on failure
 @Description	Allows textures to be stored in C header files and loaded in.  Can load parts of a
				mipmaped texture (ie skipping the highest detailed levels).  Release texture by calling
				PVRTReleaseTexture. This function is the copy of PVRTLoadPartialTextureFromPointer, hence
				when PVRTLoadPartialTextureFromPointer is changed, this function is changed too.
				In OpenGL Cube Map, each texture's up direction is defined as next (view direction, up direction),
				(+x,-y)(-x,-y)(+y,+z)(-y,-z)(+z,-y)(-z,-y).
*****************************************************************************/
unsigned int PVRTLoadPartialTextureFromPointer(const void * const pointer,
											   const void * const texPtr,
											   const unsigned int nLoadFromLevel,
											   GLuint * const texName,
											   const void *psTextureHeader)
{
	while(glGetError() != GL_NO_ERROR);

	PVR_Texture_Header* psPVRHeader = (PVR_Texture_Header*)pointer;
	unsigned int u32NumSurfs;

	// perform checks for old PVR psPVRHeader
	if(psPVRHeader->dwHeaderSize!=sizeof(PVR_Texture_Header))
	{	// Header V1
		if(psPVRHeader->dwHeaderSize==PVRTEX_V1_HEADER_SIZE)
		{	// react to old psPVRHeader: i.e. fill in numsurfs as this is missing from old header
			PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer warning: this is an old pvr"
				" - you can use PVRTexTool to update its header.\n");
			if(psPVRHeader->dwpfFlags&PVRTEX_CUBEMAP)
				u32NumSurfs = 6;
			else
				u32NumSurfs = 1;
		}
		else
		{	// not a pvr at all
			PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: not a valid pvr.\n");
			return 0;
		}
	}
	else
	{	// Header V2
		if(psPVRHeader->dwNumSurfs<1)
		{	// encoded with old version of PVRTexTool before zero numsurfs bug found.
			if(psPVRHeader->dwpfFlags & PVRTEX_CUBEMAP)
				u32NumSurfs = 6;
			else
				u32NumSurfs = 1;
		}
		else
		{
			u32NumSurfs = psPVRHeader->dwNumSurfs;
		}
	}

	GLuint textureName;
	GLenum textureFormat = 0;
	GLenum textureType = GL_RGB;

#ifdef RUDE_OGLES
	bool IsPVRTCSupported = true; //CPVRTglesExt::IsGLExtensionSupported("GL_IMG_texture_compression_pvrtc");
#else
	bool IsPVRTCSupported = false;
#endif
	
	bool IsBGRA8888Supported  = true; //CPVRTglesExt::IsGLExtensionSupported("GL_IMG_texture_format_BGRA8888");

	*texName = 0;	// install warning value
	bool IsCompressedFormatSupported = false, IsCompressedFormat = false;

	/* Only accept untwiddled data UNLESS texture format is PVRTC */
	if ( ((psPVRHeader->dwpfFlags & PVRTEX_TWIDDLE) == PVRTEX_TWIDDLE)
		&& ((psPVRHeader->dwpfFlags & PVRTEX_PIXELTYPE)!=OGL_PVRTC2)
		&& ((psPVRHeader->dwpfFlags & PVRTEX_PIXELTYPE)!=OGL_PVRTC4) )
	{
		// We need to load untwiddled textures -- hw will twiddle for us.
		PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: texture should be untwiddled.\n");
		return 0;
	}

	switch(psPVRHeader->dwpfFlags & PVRTEX_PIXELTYPE)
	{
	case OGL_RGBA_4444:
		textureFormat = GL_UNSIGNED_SHORT_4_4_4_4;
		textureType = GL_RGBA;
		break;

	case OGL_RGBA_5551:
		textureFormat = GL_UNSIGNED_SHORT_5_5_5_1;
		textureType = GL_RGBA;
		break;

	case OGL_RGBA_8888:
		textureFormat = GL_UNSIGNED_BYTE;
		textureType = GL_RGBA;
		break;

	/* New OGL Specific Formats Added */

	case OGL_RGB_565:
		textureFormat = GL_UNSIGNED_SHORT_5_6_5;
		textureType = GL_RGB;
		break;

	case OGL_RGB_555:
		PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: pixel type OGL_RGB_555 not supported.\n");
		return 0; // Deal with exceptional case

	case OGL_RGB_888:
		textureFormat = GL_UNSIGNED_BYTE;
		textureType = GL_RGB;
		break;

	case OGL_I_8:
		textureFormat = GL_UNSIGNED_BYTE;
		textureType = GL_LUMINANCE;
		break;

	case OGL_AI_88:
		textureFormat = GL_UNSIGNED_BYTE;
		textureType = GL_LUMINANCE_ALPHA;
		break;

	case OGL_PVRTC2:
		if(IsPVRTCSupported)
		{
			IsCompressedFormatSupported = IsCompressedFormat = true;
			textureFormat = psPVRHeader->dwAlphaBitMask==0 ? GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG ;	// PVRTC2
		}
		else
		{
			IsCompressedFormatSupported = false;
			IsCompressedFormat = true;
			textureFormat = GL_UNSIGNED_BYTE;
			textureType = GL_RGBA;
			PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer warning: PVRTC2 not supported. Converting to RGBA8888 instead.\n");
		}
		break;

	case OGL_PVRTC4:
		if(IsPVRTCSupported)
		{
			IsCompressedFormatSupported = IsCompressedFormat = true;
			textureFormat = psPVRHeader->dwAlphaBitMask==0 ? GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG ;	// PVRTC4
		}
		else
		{
			IsCompressedFormatSupported = false;
			IsCompressedFormat = true;
			textureFormat = GL_UNSIGNED_BYTE;
			textureType = GL_RGBA;
			PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer warning: PVRTC4 not supported. Converting to RGBA8888 instead.\n");
		}
		break;

	case OGL_BGRA_8888:
		if(IsBGRA8888Supported)
		{
			textureFormat = GL_UNSIGNED_BYTE;
			textureType   = GL_BGRA;
			break;
		}
		else
		{
			PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: Unable to load GL_BGRA texture as extension GL_IMG_texture_format_BGRA8888 is unsupported.\n");
			return 0;
		}
	default:											// NOT SUPPORTED
		PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: pixel type not supported.\n");
		return 0;
	}

	// load the texture up
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);				// Never have row-aligned in psPVRHeaders

	glGenTextures(1, &textureName);

	//  check that this data is cube map data or not.
	if(psPVRHeader->dwpfFlags & PVRTEX_CUBEMAP)
	{ // not in OGLES you don't
		PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: cube map textures are not available in OGLES1.x.\n");
		return 0;
	}
	else
	{
		while(glGetError() != GL_NO_ERROR);
		glBindTexture(GL_TEXTURE_2D, textureName);
	}

	int error = glGetError();
	if(error)
	{
		printf("Unable to bind texture: 0x%x\n", error);
		PVRTOOLS_DEBUG_OUTPUT("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: glBindTexture() failed.\n");
		return 0;
	}

	for(unsigned int i=0; i<u32NumSurfs; i++)
	{
		char *theTexturePtr = (texPtr? (char*)texPtr :  (char*)psPVRHeader + psPVRHeader->dwHeaderSize) + psPVRHeader->dwTextureDataSize * i;
		char *theTextureToLoad = 0;
		int		nMIPMapLevel;
		int		nTextureLevelsNeeded = (psPVRHeader->dwpfFlags & PVRTEX_MIPMAP)? psPVRHeader->dwMipMapCount : 0;
		unsigned int		nSizeX= psPVRHeader->dwWidth, nSizeY = psPVRHeader->dwHeight;
		unsigned int		CompressedImageSize = 0;

		for(nMIPMapLevel = 0; nMIPMapLevel <= nTextureLevelsNeeded; nSizeX = PVRT_MAX(nSizeX/2, (unsigned int)1), nSizeY = PVRT_MAX(nSizeY/2, (unsigned int)1), nMIPMapLevel++)
		{
			// Do Alpha-swap if needed

			theTextureToLoad = theTexturePtr;

			// Load the Texture

			/* If the texture is PVRTC then use GLCompressedTexImage2D */
			if(IsCompressedFormat)
			{
				/* Calculate how many bytes this MIP level occupies */
				if ((psPVRHeader->dwpfFlags & PVRTEX_PIXELTYPE)==OGL_PVRTC2)
				{
					CompressedImageSize = ( PVRT_MAX(nSizeX, PVRTC2_MIN_TEXWIDTH) * PVRT_MAX(nSizeY, PVRTC2_MIN_TEXHEIGHT) * psPVRHeader->dwBitCount + 7) / 8;
				}
				else
				{// PVRTC4 case
					CompressedImageSize = ( PVRT_MAX(nSizeX, PVRTC4_MIN_TEXWIDTH) * PVRT_MAX(nSizeY, PVRTC4_MIN_TEXHEIGHT) * psPVRHeader->dwBitCount + 7) / 8;
				}

				if(((signed int)nMIPMapLevel - (signed int)nLoadFromLevel) >= 0)
				{
					if(IsCompressedFormatSupported)
					{
						//if(psPVRHeader->dwpfFlags&PVRTEX_CUBEMAP)
						//{
						//	/* Load compressed texture data at selected MIP level */
						//	glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, nMIPMapLevel-nLoadFromLevel, textureFormat, nSizeX, nSizeY, 0,
						//					CompressedImageSize, theTextureToLoad);
						//}
						//else
						{
							/* Load compressed texture data at selected MIP level */
							glCompressedTexImage2D(GL_TEXTURE_2D, nMIPMapLevel-nLoadFromLevel, textureFormat, nSizeX, nSizeY, 0,
											CompressedImageSize, theTextureToLoad);

						}
					}
					else
					{
						// Convert PVRTC to 32-bit
						unsigned char *u8TempTexture = (unsigned char*)malloc(nSizeX*nSizeY*4);
						if ((psPVRHeader->dwpfFlags & PVRTEX_PIXELTYPE)==OGL_PVRTC2)
						{
							PVRTCDecompress(theTextureToLoad, 1, nSizeX, nSizeY, u8TempTexture);
						}
						else
						{// PVRTC4 case
							PVRTCDecompress(theTextureToLoad, 0, nSizeX, nSizeY, u8TempTexture);
						}


						//if(psPVRHeader->dwpfFlags&PVRTEX_CUBEMAP)
						//{// Load compressed cubemap data at selected MIP level
						//	// Upload the texture as 32-bits
						//	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,nMIPMapLevel-nLoadFromLevel,GL_RGBA,
						//		nSizeX,nSizeY,0, GL_RGBA,GL_UNSIGNED_BYTE,u8TempTexture);
						//	FREE(u8TempTexture);
						//}
						//else
						{// Load compressed 2D data at selected MIP level
							// Upload the texture as 32-bits
							glTexImage2D(GL_TEXTURE_2D,nMIPMapLevel-nLoadFromLevel,GL_RGBA,
								nSizeX,nSizeY,0, GL_RGBA,GL_UNSIGNED_BYTE,u8TempTexture);
							FREE(u8TempTexture);
						}
					}
				}
			}
			else
			{
				if(((signed int)nMIPMapLevel - (signed int)nLoadFromLevel) >= 0)
				{
					unsigned int pixelType = psPVRHeader->dwpfFlags & PVRTEX_PIXELTYPE;
					if(pixelType == OGL_RGBA_5551)
					{
						unsigned char *u8Temp = (unsigned char *)malloc(nSizeX * nSizeY * 4);
						if(!u8Temp)
						{
							printf("PVRTTexture: Out of memory unpacking OGL_RGBA_5551\n");
							return 0;
						}
						const unsigned char *src = (const unsigned char *)theTextureToLoad;
						for(unsigned int p = 0; p < nSizeX * nSizeY; ++p)
						{
							unsigned short val = (unsigned short)(src[p * 2] | (src[p * 2 + 1] << 8));
							u8Temp[p * 4 + 0] = (unsigned char)((((val >> 11) & 0x1F) * 255 + 15) / 31);
							u8Temp[p * 4 + 1] = (unsigned char)((((val >> 6) & 0x1F) * 255 + 15) / 31);
							u8Temp[p * 4 + 2] = (unsigned char)((((val >> 1) & 0x1F) * 255 + 15) / 31);
							u8Temp[p * 4 + 3] = (val & 1) ? 255 : 0;
						}
						glTexImage2D(GL_TEXTURE_2D, nMIPMapLevel - nLoadFromLevel, GL_RGBA,
									 nSizeX, nSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, u8Temp);
						FREE(u8Temp);
					}
					else if(pixelType == OGL_RGBA_4444)
					{
						unsigned char *u8Temp = (unsigned char *)malloc(nSizeX * nSizeY * 4);
						if(!u8Temp)
						{
							printf("PVRTTexture: Out of memory unpacking OGL_RGBA_4444\n");
							return 0;
						}
						const unsigned char *src = (const unsigned char *)theTextureToLoad;
						for(unsigned int p = 0; p < nSizeX * nSizeY; ++p)
						{
							unsigned short val = (unsigned short)(src[p * 2] | (src[p * 2 + 1] << 8));
							u8Temp[p * 4 + 0] = (unsigned char)((((val >> 12) & 0x0F) * 255 + 7) / 15);
							u8Temp[p * 4 + 1] = (unsigned char)((((val >> 8) & 0x0F) * 255 + 7) / 15);
							u8Temp[p * 4 + 2] = (unsigned char)((((val >> 4) & 0x0F) * 255 + 7) / 15);
							u8Temp[p * 4 + 3] = (unsigned char)(((val & 0x0F) * 255 + 7) / 15);
						}
						glTexImage2D(GL_TEXTURE_2D, nMIPMapLevel - nLoadFromLevel, GL_RGBA,
									 nSizeX, nSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, u8Temp);
						FREE(u8Temp);
					}
					else if(pixelType == OGL_RGB_565)
					{
						unsigned char *u8Temp = (unsigned char *)malloc(nSizeX * nSizeY * 4);
						if(!u8Temp)
						{
							printf("PVRTTexture: Out of memory unpacking OGL_RGB_565\n");
							return 0;
						}
						const unsigned char *src = (const unsigned char *)theTextureToLoad;
						for(unsigned int p = 0; p < nSizeX * nSizeY; ++p)
						{
							unsigned short val = (unsigned short)(src[p * 2] | (src[p * 2 + 1] << 8));
							u8Temp[p * 4 + 0] = (unsigned char)((((val >> 11) & 0x1F) * 255 + 15) / 31);
							u8Temp[p * 4 + 1] = (unsigned char)((((val >> 5) & 0x3F) * 255 + 31) / 63);
							u8Temp[p * 4 + 2] = (unsigned char)((((val) & 0x1F) * 255 + 15) / 31);
							u8Temp[p * 4 + 3] = 255;
						}
						glTexImage2D(GL_TEXTURE_2D, nMIPMapLevel - nLoadFromLevel, GL_RGBA,
									 nSizeX, nSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, u8Temp);
						FREE(u8Temp);
					}
					else if(pixelType == OGL_RGB_888)
					{
						unsigned char *u8Temp = (unsigned char *)malloc(nSizeX * nSizeY * 4);
						if(!u8Temp)
						{
							printf("PVRTTexture: Out of memory unpacking OGL_RGB_888\n");
							return 0;
						}
						const unsigned char *src = (const unsigned char *)theTextureToLoad;
						for(unsigned int p = 0; p < nSizeX * nSizeY; ++p)
						{
							u8Temp[p * 4 + 0] = src[p * 3 + 2]; // R
							u8Temp[p * 4 + 1] = src[p * 3 + 1]; // G
							u8Temp[p * 4 + 2] = src[p * 3 + 0]; // B
							u8Temp[p * 4 + 3] = 255;            // A
						}
						glTexImage2D(GL_TEXTURE_2D, nMIPMapLevel - nLoadFromLevel, GL_RGBA,
									 nSizeX, nSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, u8Temp);
						FREE(u8Temp);
					}
					else
					{
						/* Load uncompressed texture data at selected MIP level */
						glTexImage2D(GL_TEXTURE_2D,nMIPMapLevel-nLoadFromLevel,textureType,nSizeX,nSizeY,0, textureType,textureFormat,theTextureToLoad);
					}
				}
			}

			int error = glGetError();
			if(error)
			{
				printf("PVRTTexture:PVRTLoadPartialTextureFromPointer failed: glTexImage2D() failed. (0x%x)\n", error);
				return 0;
			}

			// offset the texture pointer by one mip-map level

			/* PVRTC case */
			if ( IsCompressedFormat )
			{
				theTexturePtr += CompressedImageSize;
			}
			else
			{
				/* New formula that takes into account bit counts inferior to 8 (e.g. 1 bpp) */
				theTexturePtr += (nSizeX * nSizeY * psPVRHeader->dwBitCount + 7) / 8;
			}
		}
	}

	*texName = textureName;

	glBindTexture(GL_TEXTURE_2D, textureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if(psPVRHeader->dwMipMapCount > 0)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#if defined(RUDE_AMIGAOS4)
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
#endif

	if(psTextureHeader)
	{
		*(PVR_Texture_Header*)psTextureHeader = *psPVRHeader;
		((PVR_Texture_Header*)psTextureHeader)->dwPVR = PVRTEX_IDENTIFIER;
		((PVR_Texture_Header*)psTextureHeader)->dwNumSurfs = u32NumSurfs;
	}

	return psPVRHeader->dwpfFlags|0x80000000;		// PVR psPVRHeader flags with topmost bit set so that it is non-zero
}
/*!***************************************************************************
 @Function		PVRTLoadTextureFromPointer
 @Input			pointer			Pointer to header-texture's structure
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in C header files and loaded in.  Loads the whole texture.
				Release texture by calling PVRTReleaseTexture.
*****************************************************************************/
unsigned int PVRTLoadTextureFromPointer(const void* pointer, GLuint *const texName, const void *psTextureHeader)
{
	return PVRTLoadPartialTextureFromPointer(pointer, 0, 0, texName, psTextureHeader);
}

/*!***************************************************************************
 @Function		PVRTLoadPartialTextureFromPVR
 @Input			filename		Filename of the .PVR file to load the texture from
 @Input			texPtr		If null, texture follows header, else texture is here.
 @Input			nLoadFromLevel	Which mipmap level to start loading from (0=all)
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in binary PVR files and loaded in. Can load parts of a
				mipmaped texture (ie skipping the highest detailed levels). Release texture by
				calling PVRTReleaseTexture.
*****************************************************************************/
unsigned int PVRTLoadPartialTextureFromPVR(const char * const filename,
										   const char * const texPtr,
										   const unsigned int nLoadFromLevel,
										   GLuint * const texName,
										   const void *psTextureHeader)
{
	CPVRTResourceFile TexFile(filename);
	if (!TexFile.IsOpen()) return 0;

#if defined(RUDE_AMIGAOS4)
	const unsigned char *source =
		static_cast<const unsigned char *>(TexFile.DataPtr());
	const size_t size = TexFile.Size();
	if(size < sizeof(PVR_Texture_Header))
		return 0;
	unsigned char *normalized = static_cast<unsigned char *>(malloc(size));
	if(normalized == NULL)
		return 0;
	memcpy(normalized, source, size);
	PVR_Texture_Header *header =
		reinterpret_cast<PVR_Texture_Header *>(normalized);
	unsigned int *fields = reinterpret_cast<unsigned int *>(header);
	for(size_t i = 0; i < sizeof(PVR_Texture_Header) / sizeof(unsigned int); ++i)
	{
		const unsigned char *raw = source + i * sizeof(unsigned int);
		fields[i] = static_cast<unsigned int>(raw[0]) |
			(static_cast<unsigned int>(raw[1]) << 8) |
			(static_cast<unsigned int>(raw[2]) << 16) |
			(static_cast<unsigned int>(raw[3]) << 24);
	}
	if(header->dwHeaderSize < sizeof(PVR_Texture_Header) ||
	   header->dwHeaderSize > size ||
	   header->dwTextureDataSize > size - header->dwHeaderSize)
	{
		free(normalized);
		return 0;
	}
	if((header->dwpfFlags & PVRTEX_PIXELTYPE) == OGL_PVRTC2 ||
	   (header->dwpfFlags & PVRTEX_PIXELTYPE) == OGL_PVRTC4)
	{
		/* PVRTC blocks are pairs of 32-bit words in little-endian;
		   swap each 4-byte group so the software decompressor reads
		   correct colour and modulation values on big-endian. */
		unsigned char *pixels = normalized + header->dwHeaderSize;
		const size_t bytes = size - header->dwHeaderSize;
		for(size_t i = 0; i + 3 < bytes; i += 4)
		{
			unsigned char b0 = pixels[i];
			unsigned char b1 = pixels[i + 1];
			pixels[i]     = pixels[i + 3];
			pixels[i + 1] = pixels[i + 2];
			pixels[i + 2] = b1;
			pixels[i + 3] = b0;
		}
	}
	unsigned int result = PVRTLoadPartialTextureFromPointer(
		normalized, texPtr, nLoadFromLevel, texName, psTextureHeader);
	free(normalized);
	return result;
#else
	return PVRTLoadPartialTextureFromPointer(TexFile.DataPtr(),
		texPtr,
		nLoadFromLevel,
		texName,
		psTextureHeader);
#endif
}

/*!***************************************************************************
 @Function		PVRTLoadTextureFromPVR
 @Input			filename		Filename of the .PVR file to load the texture from
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in binary PVR files and loaded in. Loads the whole texture
				Release texture by calling PVRTReleaseTexture.
*****************************************************************************/
unsigned int PVRTLoadTextureFromPVR(const char * const filename, GLuint * const texName, const void *psTextureHeader)
{
	return PVRTLoadPartialTextureFromPVR(filename, 0, 0, texName, psTextureHeader);
}

/*!***************************************************************************
 @Function		PVRTLoadDecompressedPartialTextureFromPVR
 @Input			filename		Filename of the .PVR file to load the texture from
 @Input			nLoadFromLevel	Which mipmap level to start loading from (0=all)
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in binary PVR files and loaded in. Can load parts of a
				mipmaped texture (ie skipping the highest detailed levels). Release texture by
				calling PVRTReleaseTexture.  This variant decompresses to RGBA8888.
*****************************************************************************/
unsigned int PVRTLoadDecompressedPartialTextureFromPVR(const char* const filename, unsigned int nLoadFromLevel, GLuint *const texName, const void *psTextureHeader)
{
	CPVRTResourceFile TexFile(filename);
	if (!TexFile.IsOpen()) return 0;

	return PVRTLoadDecompressedPartialTextureFromPointer(TexFile.DataPtr(),
		nLoadFromLevel,
		texName,
		psTextureHeader);
}

/*!***************************************************************************
 @Function		PVRTLoadDecompressedTextureFromPVR
 @Input			filename		Filename of the .PVR file to load the texture from
 @Modified		texName			the OpenGL ES texture name as returned by glBindTexture
 @Modified		psTextureHeader	Pointer to a PVR_Texture_Header struct. Modified to
								contain the header data of the returned texture Ignored if NULL.
 @Return		true on success
 @Description	Allows textures to be stored in binary PVR files and loaded in. Loads the whole texture
				Release texture by calling PVRTReleaseTexture.  This variant decompresses to RGBA8888.
*****************************************************************************/
unsigned int PVRTLoadDecompressedTextureFromPVR(const char* const filename, GLuint *texName, const void *psTextureHeader)
{
	return PVRTLoadDecompressedPartialTextureFromPVR(filename, 0, texName, psTextureHeader);
}

/*!***************************************************************************
 @Function		PVRTReleaseTexture
 @Input			texName			the name returned by PVRTLoadTextureFromPointer or
								PVRTLoadTextureFromPVR
 @Description	Releases the resources used by a texture.
*****************************************************************************/
void PVRTReleaseTexture(GLuint texName)
{
	glDeleteTextures(1,&texName);
}

/*!***************************************************************************
 @Function			PVRTTextureFormatBPP
 @Input				nFormat
 @Input				nType
 @Description		Returns the bits per pixel (BPP) of the format.
*****************************************************************************/
unsigned int PVRTTextureFormatBPP(const GLuint nFormat, const GLuint nType)
{
	switch(nFormat)
	{
	case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
		return 2;
	case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
		return 4;
	case GL_UNSIGNED_BYTE:
		switch(nType)
		{
		case GL_RGBA:
		case GL_BGRA:
			return 32;
		}
	case GL_UNSIGNED_SHORT_5_5_5_1:
		switch(nType)
		{
		case GL_RGBA:
			return 16;
		}
	}

	return 0xFFFFFFFF;
}

/*****************************************************************************
 End of file (PVRTTextureAPI.cpp)
*****************************************************************************/
