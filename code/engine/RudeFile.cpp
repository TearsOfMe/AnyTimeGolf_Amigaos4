/*
 *  RudeFile.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeFile.h"

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
#include <CoreFoundation/CFBundle.h>
#endif

/**
 * Returns the full location of a file in a cross-platform compatible manner.
 *
 * @param filename [in] The filename
 * @param buffer [out] The path to the file for loading purposes
 * @param bufsize [in] The size of buffer
 * @param canfail [in] True if this operation can fail
 *
 * @returns True if the file was found, false otherwise
 */
bool RudeFileGetFile(const char *filename, char *buffer, int bufsize, bool canfail)
{
	
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	CFStringRef cfFilename = CFStringCreateWithCString(0, filename, kCFStringEncodingASCII);
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourceURL(mainBundle, cfFilename, 0, 0);
	
	if(url == NULL)
	{
		buffer[0] = '\0';
		RUDE_ASSERT(canfail, "Could not locate file %s", filename);
		return false;
	}
	
	CFURLGetFileSystemRepresentation(url, true, (UInt8 *) buffer, bufsize);
	
	CFRelease(url);
	CFRelease(cfFilename);
	
	return true;
#endif // defined(RUDE_IPHONE) || defined(RUDE_MACOS)

#ifdef RUDE_WIN
	_snprintf(buffer, bufsize, "data\\%s", filename);
	
	DWORD fileattr = GetFileAttributes(buffer);

	if(fileattr == 0xFFFFFFFF)
	{
		buffer[0] = '\0';
		RUDE_ASSERT(canfail, "Could not locate file %s", filename);
		return false;
	}

	return true;
#endif

#ifdef RUDE_AMIGAOS4
	if(filename == NULL || buffer == NULL || bufsize <= 0)
		return false;
	const char *formats[] = {"PROGDIR:data/%s", "data/%s", "%s", "PROGDIR:%s"};
	for(unsigned int i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i)
	{
		int written = snprintf(buffer, (size_t)bufsize, formats[i], filename);
		if(written < 0 || written >= bufsize)
			continue;

		FILE *file = fopen(buffer, "rb");
		if(file != NULL)
		{
			fclose(file);
			return true;
		}
	}

	buffer[0] = '\0';
	RUDE_ASSERT(canfail, "Could not locate file %s", filename);
	return false;
#endif

	buffer[0] = '\0';
	RUDE_ASSERT(canfail, "Unsupported platform while locating file %s", filename);
	return false;
}
