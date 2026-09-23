/*
 *  RudeMesh.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeMesh.h"
#include "RudeGL.h"
#include "RudeTextureManager.h"
#include "RudeFile.h"
#include "RudeDebug.h"



RudeMesh::RudeMesh(RudeObject *owner)
: m_owner(owner)
, m_scale(1.0f, 1.0f, 1.0f)
, m_textureOverride(false)
{
	for(int i = 0; i < kMaxNodes; i++)
		m_colorOverrides[i] = 0;
	
	for(int i = 0; i < kMaxTextures; i++)
		m_textureOverrides[i] = -1;
}

RudeMesh::~RudeMesh()
{
}

int RudeMesh::Load(const char *name)
{
	RUDE_ASSERT(name, "Loading mesh with no name");
	
	RUDE_REPORT("RudeMesh::Load %s\n", name);
	
	char filename[64];
	snprintf(filename, sizeof(filename), "%s.POD", name);
	
	char modelfile[512];
	if(!RudeFileGetFile(filename, modelfile, sizeof(modelfile), false))
		return -1;
	
	int result = m_model.ReadFromFile(modelfile, 0, 0);
	
	RUDE_ASSERT(result == 1, "Could not load model");
	
	if(result == 0)
		return -1;
	
	RUDE_ASSERT(m_model.nNumTexture < kMaxTextures, "Too many textures in model");
	for(unsigned int i = 0; i < m_model.nNumTexture; i++)
	{
		SPODTexture *texture = &m_model.pTexture[i];
		RUDE_ASSERT(texture, "Invalid texture in model");
		
		char texturename[64];
		snprintf(texturename, sizeof(texturename), "%s", texture->pszName);
		char *extension = strrchr(texturename, '.');
		if(extension != NULL)
			*extension = '\0';
		
		m_textures[i] = RudeTextureManager::GetInstance()->LoadTextureFromPVRTFile(texturename);
		RUDE_ASSERT(m_textures[i] >= 0, "Could not load texture");
		
	}
	
	// make sure we have at least one renderable node
	bool foundRenderable = false;
	for(unsigned int i = 0; i < m_model.nNumNode; i++)
	{
		SPODNode *node = &m_model.pNode[i];
		
		if(!node->pszName)
			continue;
		if(node->pszName[0] != 'M')
			continue;
		
		foundRenderable = true;
		
		RUDE_REPORT("  Node %s: mesh %d\n", node->pszName, node->nIdx);
	}
	
	RUDE_ASSERT(foundRenderable, "Didn't find any renderable meshes in %s", name);

	// --- CONVERT NON-FLOAT MESHES TO FLOAT ARRAYS ---
	for(unsigned int i = 0; i < m_model.nNumMesh; i++) {
		SPODMesh *mesh = &m_model.pMesh[i];
		
		int oldType = mesh->sVertex.eType;
		
		if(mesh->sVertex.eType != EPODDataFloat && mesh->sVertex.eType != 0 && mesh->nNumVertex > 0) {
			
			int newStride = 0;
			if(mesh->sVertex.n > 0) newStride += mesh->sVertex.n * sizeof(float);
			if(mesh->sNormals.n > 0) newStride += mesh->sNormals.n * sizeof(float);
			if(mesh->nNumUVW > 0 && mesh->psUVW != NULL && mesh->psUVW[0].n > 0) newStride += mesh->psUVW[0].n * sizeof(float);
			if(mesh->sVtxColours.n > 0) newStride += 4; // RGBA bytes
			
			unsigned char* newInterleaved = (unsigned char*)malloc(mesh->nNumVertex * newStride);
			
			for(unsigned int v = 0; v < mesh->nNumVertex; v++) {
				unsigned char* oldBase = mesh->pInterleaved + v * mesh->sVertex.nStride;
				unsigned char* newBase = newInterleaved + v * newStride;
				int outOffset = 0;
				
				// Convert Vertices
				if(mesh->sVertex.n > 0) {
					unsigned char* src = oldBase + (long)mesh->sVertex.pData;
					float* dst = (float*)(newBase + outOffset);
					for(unsigned int c = 0; c < mesh->sVertex.n; c++) {
						float val = 0.0f;
						if(mesh->sVertex.eType == EPODDataFixed16_16) {
							val = (float)(*((int*)(src + c*4))) / 65536.0f;
						} else if(mesh->sVertex.eType == EPODDataShort) {
							val = (float)(*((short*)(src + c*2)));
						} else if(mesh->sVertex.eType == EPODDataShortNorm) {
							val = (float)(*((short*)(src + c*2))) / 32767.0f;
						} else if(mesh->sVertex.eType == EPODDataFloat) {
						    val = *((float*)(src + c*4));
						}
						dst[c] = val;
					}
					outOffset += mesh->sVertex.n * sizeof(float);
				}
				
				// Convert Normals
				if(mesh->sNormals.n > 0) {
					unsigned char* src = oldBase + (long)mesh->sNormals.pData;
					float* dst = (float*)(newBase + outOffset);
					for(unsigned int c = 0; c < mesh->sNormals.n; c++) {
						float val = 0.0f;
						if(mesh->sNormals.eType == EPODDataFixed16_16) {
							val = (float)(*((int*)(src + c*4))) / 65536.0f;
						} else if(mesh->sNormals.eType == EPODDataShort) {
							val = (float)(*((short*)(src + c*2)));
						} else if(mesh->sNormals.eType == EPODDataShortNorm) {
							val = (float)(*((short*)(src + c*2))) / 32767.0f;
						} else if(mesh->sNormals.eType == EPODDataFloat) {
							val = *((float*)(src + c*4));
						}
						dst[c] = val;
					}
					outOffset += mesh->sNormals.n * sizeof(float);
				}
				
				// Convert UVs
				if(mesh->nNumUVW > 0 && mesh->psUVW != NULL && mesh->psUVW[0].n > 0) {
					unsigned char* src = oldBase + (long)mesh->psUVW[0].pData;
					float* dst = (float*)(newBase + outOffset);
					for(unsigned int c = 0; c < mesh->psUVW[0].n; c++) {
						float val = 0.0f;
						if(mesh->psUVW[0].eType == EPODDataFixed16_16) {
							val = (float)(*((int*)(src + c*4))) / 65536.0f;
						} else if(mesh->psUVW[0].eType == EPODDataShort) {
							val = (float)(*((short*)(src + c*2)));
						} else if(mesh->psUVW[0].eType == EPODDataShortNorm) {
							val = (float)(*((short*)(src + c*2))) / 32767.0f;
						} else if(mesh->psUVW[0].eType == EPODDataFloat) {
							val = *((float*)(src + c*4));
						}
						dst[c] = val;
					}
					outOffset += mesh->psUVW[0].n * sizeof(float);
				}
				
				// Copy Vertex Colors
				if(mesh->sVtxColours.n > 0) {
				    unsigned char* src = oldBase + (long)mesh->sVtxColours.pData;
				    unsigned char* dst = newBase + outOffset;
				    dst[0] = src[0];
				    dst[1] = src[1];
				    dst[2] = src[2];
				    dst[3] = src[3];
				    outOffset += 4;
				}
			}
			
			// Replace interleaved buffer
			free(mesh->pInterleaved);
			mesh->pInterleaved = newInterleaved;
			mesh->sVertex.nStride = newStride;
			
			int offset = 0;
			if(mesh->sVertex.n > 0) {
				mesh->sVertex.pData = (unsigned char*)(long)offset;
				mesh->sVertex.eType = EPODDataFloat;
				offset += mesh->sVertex.n * sizeof(float);
			}
			if(mesh->sNormals.n > 0) {
				mesh->sNormals.pData = (unsigned char*)(long)offset;
				mesh->sNormals.eType = EPODDataFloat;
				offset += mesh->sNormals.n * sizeof(float);
			}
			if(mesh->nNumUVW > 0 && mesh->psUVW != NULL && mesh->psUVW[0].n > 0) {
				mesh->psUVW[0].pData = (unsigned char*)(long)offset;
				mesh->psUVW[0].eType = EPODDataFloat;
				offset += mesh->psUVW[0].n * sizeof(float);
			}
			if(mesh->sVtxColours.n > 0) {
			    mesh->sVtxColours.pData = (unsigned char*)(long)offset;
			    offset += 4;
			}
		}
		
		// Fix vertex colors for Big-Endian AmigaOS
		// In original POD data on Big-Endian, byte 0 is 0x00 and byte 1/2 hold grayscale light intensity.
		// Set R=G=B=intensity, A=255 so OpenGL texture modulation preserves true texture colors.
		if(mesh->sVtxColours.n > 0)
		{
			unsigned char *base = mesh->pInterleaved ? (mesh->pInterleaved + (long)mesh->sVtxColours.pData) : (unsigned char*)mesh->sVtxColours.pData;
			if(base)
			{
				unsigned char *c = base;
				for(unsigned int j = 0; j < mesh->nNumVertex; j++)
				{
					unsigned char intensity = c[1];
					if(intensity == 0 && c[2] != 0) intensity = c[2];
					if(intensity == 0) intensity = 255;
					c[0] = intensity;
					c[1] = intensity;
					c[2] = intensity;
					c[3] = 255;
					c += mesh->sVtxColours.nStride;
				}
			}
		}
	}

	return 0;
}

void RudeMesh::AddTextureOverride(const char *oldTexture, const char *newTexture)
{
	bool found = false;
	
	for(unsigned int i = 0; i < m_model.nNumTexture; i++)
	{
		SPODTexture *texture = &m_model.pTexture[i];
		RUDE_ASSERT(texture, "Invalid texture in model");
		
		char texturename[64];
		snprintf(texturename, sizeof(texturename), "%s", texture->pszName);
		char *extension = strrchr(texturename, '.');
		if(extension != NULL)
			*extension = '\0';
		
		if(strcmp(oldTexture, texturename) == 0)
		{
			m_textureOverrides[i] = RudeTextureManager::GetInstance()->LoadTextureFromPVRTFile(newTexture);
			RUDE_ASSERT(m_textureOverrides[i] >= 0, "Could not load texture %s", newTexture);
			found = true;
		}
	}

	RUDE_ASSERT(found, "Texture %s not found", oldTexture);
}

void RudeMesh::SetColorOverride(int node, const char *colordata)
{
	RUDE_ASSERT(node < kMaxNodes, "Invalid node");
	
	m_colorOverrides[node] = colordata;
}

void RudeMesh::EnableModel(int n, bool enable)
{
	bool found = false;
	for(unsigned int i = 0; i < m_model.nNumNode; i++)
	{
		SPODNode *node = &m_model.pNode[i];
		
		if(!node->pszName)
			continue;
		
		if(node->pszName[0] == 'M' || node->pszName[0] == 'm')
		{
			if(node->pszName[1] == ('0' + n))
			{
				found = true;
				
				if(enable)
					node->pszName[0] = 'M';
				else
					node->pszName[0] = 'm';
			}
		}
	}
	
	RUDE_ASSERT(found, "Could not find model number %d", n);
	
}

void RudeMesh::Render()
{

	
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	RGL.EnableClient(kVertexArray, true);
	RGL.EnableClient(kTextureCoordArray, true);
	
	//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	
	//glScalef(m_scale.x(), m_scale.y(), m_scale.z());
	
	for(unsigned int i = 0; i < m_model.nNumNode; i++)
	{
		SPODNode *node = &m_model.pNode[i];
		
		if(!node->pszName)
			continue;
		if(node->pszName[0] != 'M')
			continue;
		
		SPODMaterial *material = &m_model.pMaterial[node->nIdxMaterial];
		SPODMesh *mesh = &m_model.pMesh[node->nIdx];
		
		int textureid = material->nIdxTexDiffuse;
		if(textureid >= 0)
		{
			if(m_textureOverride && m_textureOverrides[textureid] >= 0)
				RudeTextureManager::GetInstance()->SetTexture(m_textureOverrides[textureid]);
			else
				RudeTextureManager::GetInstance()->SetTexture(m_textures[textureid]);
		}
		
		unsigned short *indices	= (unsigned short*) mesh->sFaces.pData;
		
		if(mesh->sVertex.eType == EPODDataShortNorm)
		{
			float s = 1.0f / 1000.0f;
			glMatrixMode(GL_MODELVIEW);
			glScalef(s, s, s);
			glVertexPointer(3, GL_UNSIGNED_SHORT, mesh->sVertex.nStride, mesh->pInterleaved + (long)mesh->sVertex.pData);
		}
		else
			glVertexPointer(3, GL_FLOAT, mesh->sVertex.nStride, mesh->pInterleaved + (long)mesh->sVertex.pData);
		
		glTexCoordPointer(2, GL_FLOAT, mesh->psUVW->nStride, mesh->pInterleaved + (long)mesh->psUVW->pData);
		
		if(m_colorOverrides[i])
		{
			RGL.EnableClient(kColorArray, true);
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_UNSIGNED_BYTE, 4, m_colorOverrides[i]);
		}
		else
		{
			if(mesh->sVtxColours.n > 0)
			{
				RGL.EnableClient(kColorArray, true);
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(4, GL_UNSIGNED_BYTE, mesh->sVtxColours.nStride, mesh->pInterleaved + (long)mesh->sVtxColours.pData);
			}
			else
			{
				RGL.EnableClient(kColorArray, false);
				glDisableClientState(GL_COLOR_ARRAY);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
		
		glDrawElements(GL_TRIANGLES, mesh->nNumFaces*3, GL_UNSIGNED_SHORT, indices);
		
		
	}
	
	RGL.EnableClient(kColorArray, false);
	glDisableClientState(GL_COLOR_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
#if 0
	
	glAlphaFunc ( GL_GREATER, 0.5 ) ;
    glEnable ( GL_ALPHA_TEST ) ;
	
	for(int i = 0; i < m_model.nNumNode; i++)
	{
		SPODNode *node = &m_model.pNode[i];
		
		if(!node->pszName)
			continue;
		if(node->pszName[0] != 'D')
			continue;
		
		SPODMaterial *material = &m_model.pMaterial[node->nIdxMaterial];
		SPODMesh *mesh = &m_model.pMesh[node->nIdx];
		
		int textureid = material->nIdxTexDiffuse;
		if(textureid >= 0)
		{
			if(m_textureOverride && m_textureOverrides[textureid] >= 0)
				RudeTextureManager::GetInstance()->SetTexture(m_textureOverrides[textureid]);
			else
				RudeTextureManager::GetInstance()->SetTexture(m_textures[textureid]);
		}
		
		unsigned short *indices	= (unsigned short*) mesh->sFaces.pData;
		
		if(mesh->sVertex.eType == EPODDataFixed16_16)
			glVertexPointer(3, GL_FIXED, mesh->sVertex.nStride, mesh->pInterleaved + (long)mesh->sVertex.pData);
		else if(mesh->sVertex.eType == EPODDataShortNorm)
		{
			float s = 1.0f / 1000.0f;
			glMatrixMode(GL_MODELVIEW);
			glScalef(s, s, s);
			glVertexPointer(3, GL_UNSIGNED_SHORT, mesh->sVertex.nStride, mesh->pInterleaved + (long)mesh->sVertex.pData);
		}
		else
			glVertexPointer(3, GL_FLOAT, mesh->sVertex.nStride, mesh->pInterleaved + (long)mesh->sVertex.pData);
		
		glTexCoordPointer(2, GL_FLOAT, mesh->psUVW->nStride, mesh->pInterleaved + (long)mesh->psUVW->pData);
		
		if(mesh->sVtxColours.n > 0)
		{
			RGL.EnableClient(kColorArray, true);
			glColorPointer(4, GL_UNSIGNED_BYTE, mesh->sVtxColours.nStride, mesh->pInterleaved + (long)mesh->sVtxColours.pData);
		}
		else
			RGL.EnableClient(kColorArray, false);
			
		glDrawElements(GL_TRIANGLES, mesh->nNumFaces*3, GL_UNSIGNED_SHORT, indices);
		
		
		
	}
	
    glDisable ( GL_ALPHA_TEST ) ;
	
#endif
		
}
