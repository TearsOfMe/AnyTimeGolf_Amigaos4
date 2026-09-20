/*
 *  RudeRegistry.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeRegistry.h"

#include <stdio.h>

#ifdef RUDE_WIN
#include "RudeRegistryWin.h"
#include "RudeRegistryText.h"
#endif

#ifdef RUDE_SYMBIAN
#include "RudeRegistrySymbian.h"
#include "RudeRegistryText.h"
#endif

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
#include "RudeRegistryCF.h"
#endif

#ifdef RUDE_AMIGAOS4
class RudeRegistryAmigaOS4 : public RudeRegistry
{
public:
	int QueryByte(const TCHAR *app, const TCHAR *name, void *buffer,
		int *buffersize)
	{
		if(app == 0 || name == 0 || buffer == 0 || buffersize == 0 ||
		   *buffersize <= 0)
			return -1;

		char filename[256];
		int written = snprintf(filename, sizeof(filename),
			"PROGDIR:save_%s_%s.dat", app, name);
		if(written < 0 || written >= (int)sizeof(filename))
			return -1;

		FILE *file = fopen(filename, "rb");
		if(file == 0)
			return -1;

		if(fseek(file, 0, SEEK_END) != 0)
		{
			fclose(file);
			return -1;
		}
		long size = ftell(file);
		if(size != *buffersize || fseek(file, 0, SEEK_SET) != 0)
		{
			fclose(file);
			return -1;
		}

		size_t bytesRead = fread(buffer, 1, (size_t)*buffersize, file);
		fclose(file);
		return bytesRead == (size_t)*buffersize ? 0 : -1;
	}

	int SetByte(const TCHAR *app, const TCHAR *name, void *buffer,
		int buffersize)
	{
		if(app == 0 || name == 0 || buffer == 0 || buffersize <= 0)
			return -1;

		char filename[256];
		int written = snprintf(filename, sizeof(filename),
			"PROGDIR:save_%s_%s.dat", app, name);
		if(written < 0 || written >= (int)sizeof(filename))
			return -1;

		FILE *file = fopen(filename, "wb");
		if(file == 0)
			return -1;

		size_t bytesWritten = fwrite(buffer, 1, (size_t)buffersize, file);
		int closeResult = fclose(file);
		return bytesWritten == (size_t)buffersize && closeResult == 0 ? 0 : -1;
	}
};
#endif

RudeRegistry::RudeRegistry(void)
{
}

RudeRegistry::~RudeRegistry(void)
{
}

/**
 * Get a pointer to the RudeRegistry singleton for the currently running platform.
 * 
 * iPhone/iPod/MacOS: RudeRegistryCF\n
 * Windows/PocketPC: RudeRegistryWin\n
 * Symbian: RudeRegistrySymbian
 */
RudeRegistry * RudeRegistry::GetSingleton()
{
	
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	
	static RudeRegistryCF *reg = 0;
	
	if(reg == 0)
		reg = new RudeRegistryCF();
	
	return (RudeRegistry *) reg;

#elif defined(RUDE_WIN)

	static RudeRegistryWin *reg = 0;

	if(reg == 0)
		reg = new RudeRegistryWin();

	return (RudeRegistry *) reg;

#elif defined(RUDE_AMIGAOS4)

	static RudeRegistryAmigaOS4 *reg = 0;
	if(reg == 0)
		reg = new RudeRegistryAmigaOS4();
	return (RudeRegistry *) reg;
	
#else
	RUDE_ASSERT(0, "RudeRegistry not defined");
	return 0;
#endif
}