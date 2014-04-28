#pragma once

#include <string>
#include <vector>
#include "D3DUtility.h"

class CD3DTerrain
{
protected:
	struct STerrainVertex
	{
		STerrainVertex () {}
		STerrainVertex (float x, float y, float z, float u, float v)
		{
			_x = x; _y = y; _z = z; _u = u; _v = v;
		}

		float _x, _y, _z, _u, _v;

		static const DWORD FVF;
	};

	IDirect3DDevice9*       m_pD3DDevice;
	IDirect3DTexture9*      m_pTexture;
	IDirect3DVertexBuffer9* m_pVertexBuff;
	IDirect3DIndexBuffer9*  m_pIndexBuff;
	D3DXMATRIX				m_matWorld;

	int m_numRow;
	int m_numCol;
	float m_rCellSpacing;
	float m_rHeightScale;

	int m_numCellsPerRow;
	int m_numCellsPerCol;
	float m_rWidth;
	float m_rDepth;
	int m_numVertices;
	int m_numTriangles;

	std::vector<float>		m_aHeightmap;

	BOOL  readPGMFile (const std::string &strHeightmapFile);
	BOOL  computeVertices ();
	BOOL  computeIndices ();

public:
	CD3DTerrain();
	~CD3DTerrain();
	BOOL loadTerrain (IDirect3DDevice9* pDevice,
		const std::string &strHeightmapFile, const std::string &strTextureFile,
		float rCellSpacing, float rHeightScale);
	BOOL  loadTexture (const std::string &strTextureFile);
	void setWorldTransform (const D3DXMATRIX &matWorld);
	BOOL renderTerrain ();
	BOOL unloadTerrain ();
	float getHeightmapEntry (int row, int col);
	void setHeightmapEntry (int row, int col, float value);
	float getHeight (float x, float z);
};
