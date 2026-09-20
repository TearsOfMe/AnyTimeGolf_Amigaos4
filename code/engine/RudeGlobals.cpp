/*
 *  RudeGlobals.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeGlobals.h"

#ifdef RUDE_IPHONE
#include <AudioToolbox/AudioQueue.h>
#endif

RudeGlobals::RudeGlobals()

{
	
}

RudeGlobals::~RudeGlobals()
{
}


static RudeGlobals *globalsinstance = 0;

RudeGlobals * RudeGlobals::GetInstance()
{
	if(globalsinstance == 0)
		globalsinstance = new RudeGlobals();
	return globalsinstance;
}


void RudeGlobals::GetPath(char *path)
{
	if(path == 0)
		return;
	path[0] = '\0';
	
#ifdef RUDE_IPHONE
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
	CFStringRef cfpath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	
	CFStringGetCString(cfpath, path, 512, kCFStringEncodingASCII);
#endif

}
