/*
 * RudeTextureManager.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeTextureManager.h"
#include <string.h>
#include <strings.h>



static RudeTextureManager *gTextureManager = 0;

RudeTextureManager * RudeTextureManager::GetInstance()
{
	if(gTextureManager == 0)
		gTextureManager = new RudeTextureManager();
	
	return gTextureManager;
}


RudeTextureManager::RudeTextureManager()
{
}

RudeTextureManager::~RudeTextureManager()
{
}

int RudeTextureManager::GetTextureID(const char *name)
{
	if(name == NULL || name[0] == '\0')
		return -1;

	unsigned int s = m_textures.size();
	
	for(unsigned int i = 0; i < s; i++)
	{
		if(m_textures[i])
		{
			const char *texname = m_textures[i]->GetName();
			if(texname == NULL)
				continue;
			
			if(strcasecmp(name, texname) == 0)
				return i;

			char b1[64], b2[64];
			strncpy(b1, name, sizeof(b1)-1); b1[sizeof(b1)-1] = '\0';
			strncpy(b2, texname, sizeof(b2)-1); b2[sizeof(b2)-1] = '\0';
			char *e1 = strrchr(b1, '.'); if(e1) *e1 = '\0';
			char *e2 = strrchr(b2, '.'); if(e2) *e2 = '\0';
			if(strcasecmp(b1, b2) == 0)
				return i;
		}
	}
	
	return -1;
}


int RudeTextureManager::LoadTextureFromPVRTFile(const char *name)
{
	RUDE_ASSERT(name, "Invalid texture name");
	if(name == 0)
		return -1;
	// check to make sure its not already loaded
	int texid = GetTextureID(name);
	if(texid >= 0)
		return texid;
	
	RudeTexture *tex = new RudeTexture();
	
	int result = tex->LoadFromPVRTFile(name);
	
	RUDE_ASSERT(result >= 0, "Unable to load texture %d (%s)", result, name);
	
	if(result < 0)
	{
		delete tex;
		return result;
	}
	
	return InsertTexture(tex);
}


int RudeTextureManager::LoadTextureFromPVRTPointer(const char *name, const void *data)
{
	RUDE_ASSERT(name && data, "Invalid embedded texture");
	if(name == 0 || data == 0)
		return -1;
	// check to make sure its not already loaded
	int texid = GetTextureID(name);
	if(texid >= 0)
		return texid;
	
	RudeTexture *tex = new RudeTexture();
	
	int result = tex->LoadFromPVRTPointer(name, data);
	
	RUDE_ASSERT(result >= 0, "Unable to load texture");
	
	if(result < 0)
	{
		delete tex;
		return result;
	}
	
	return InsertTexture(tex);
}


int RudeTextureManager::LoadTextureFromPNGFile(const char *name)
{
	RUDE_ASSERT(name, "Invalid texture name");
	
	// check to make sure its not already loaded
	int texid = GetTextureID(name);
	if(texid >= 0)
		return texid;
	
	RudeTexture *tex = new RudeTexture();
	
	int result = tex->LoadFromPNG(name);
	
	RUDE_ASSERT(result >= 0, "Unable to load texture (%s)", name);
	
	if(result < 0)
	{
		delete tex;
		return result;
	}
	
	return InsertTexture(tex);
}

int RudeTextureManager::ReplaceTextureFromPNGFile(int texid, const char *name)
{
	RUDE_ASSERT(name, "Invalid texture name");
	RUDE_ASSERT(texid >= 0, "Invalid id");
	RUDE_ASSERT(((unsigned int) texid) < m_textures.size(), "Invalid id");
	RUDE_ASSERT(m_textures[texid], "Invalid texture");
	
	int result = m_textures[texid]->LoadFromPNG(name);
	RUDE_ASSERT(result >= 0, "Unable to replace texture (%s)", name);
	if(result < 0)
		return result;
	
	return texid;
	
}

void RudeTextureManager::ReleaseTexture(int texid)
{
	RUDE_ASSERT(texid >= 0, "Invalid id");
	RUDE_ASSERT(((unsigned int) texid) < m_textures.size(), "Invalid id");
	
	if(m_textures[texid])
	{
		delete m_textures[texid];
		m_textures[texid] = 0;
	}
}

int RudeTextureManager::InsertTexture(RudeTexture *texture)
{
	unsigned int size = m_textures.size();
	for(unsigned int i = 0; i < size; i++)
	{
		if(m_textures[i] == 0)
		{
			m_textures[i] = texture;
			return i;
		}
	}
	
	m_textures.push_back(texture);	
	return m_textures.size() - 1;
}


