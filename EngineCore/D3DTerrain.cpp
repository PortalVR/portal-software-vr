#include <iostream>
#include <fstream>
#include "D3DTerrain.h"

using namespace std;

const DWORD CD3DTerrain::STerrainVertex::FVF = D3DFVF_XYZ | D3DFVF_TEX1;

float linearInterpolate (float a, float b, float t)
{
	return a - (a * t) + (b * t);
}

CD3DTerrain::CD3DTerrain ()
{
	m_pD3DDevice = NULL;
	m_pTexture = NULL;
	m_pVertexBuff = NULL;
	m_pIndexBuff = NULL;

	// initialize world matrix - position the terrain in world space
	D3DXMatrixTranslation(&m_matWorld,  0.0f, 0.0f,  0.0f);
}

CD3DTerrain::~CD3DTerrain()
{
	unloadTerrain ();
}

BOOL CD3DTerrain::loadTerrain (IDirect3DDevice9* pDevice,
	const std::string &strHeightmapFile, const std::string &strTextureFile,
	float rCellSpacing, float rHeightScale)
{
	m_pD3DDevice  = pDevice;
	m_rCellSpacing = rCellSpacing;
	m_rHeightScale = rHeightScale;

	// load heightmap
	if (FALSE == readPGMFile (strHeightmapFile))
	{
		return FALSE;
	}

#ifdef _DEBUG
	cout << "Loaded terrain height map: " << strHeightmapFile << endl;
#endif	

	// scale heights
	for (unsigned int i = 0; i < m_aHeightmap.size(); i++)
	{
		m_aHeightmap[i] *= m_rHeightScale;
	}

	// load texture
	if (FALSE == loadTexture (strTextureFile))
	{
		return FALSE;
	}

#ifdef _DEBUG
	cout << "Loaded terrain texture: " << strTextureFile << endl;
#endif	

	// compute the vertices and fill vertex buffer
	if (FALSE == computeVertices())
	{
		return FALSE;
	}

	// compute the indices and fill index buffer
	if (FALSE == computeIndices())
	{
		return FALSE;
	}

	return TRUE;
}

void CD3DTerrain::setWorldTransform (const D3DXMATRIX &matWorld)
{
	m_matWorld = matWorld;
}

BOOL CD3DTerrain::renderTerrain()
{
	if ( m_pD3DDevice )
	{
		m_pD3DDevice->SetTransform(D3DTS_WORLD, &m_matWorld);

		m_pD3DDevice->SetStreamSource(0, m_pVertexBuff, 0, sizeof(STerrainVertex));
		m_pD3DDevice->SetFVF(STerrainVertex::FVF);
		m_pD3DDevice->SetIndices(m_pIndexBuff);

		m_pD3DDevice->SetMaterial(&WHITE_MTRL);
		m_pD3DDevice->SetTexture(0, m_pTexture);

		if(FAILED(m_pD3DDevice->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST,
			0,
			0,
			m_numVertices,
			0,
			m_numTriangles)))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CD3DTerrain::unloadTerrain ()
{
	if (m_pTexture)
	{
		m_pTexture->Release ();
		m_pTexture = NULL;
	}

	if (m_pVertexBuff)
	{
		m_pVertexBuff->Release ();
		m_pVertexBuff = NULL;
	}

	if (m_pIndexBuff)
	{
		m_pIndexBuff->Release ();
		m_pIndexBuff = NULL;
	}

	m_aHeightmap.clear ();

	return TRUE;
}

BOOL CD3DTerrain::readPGMFile (const std::string &strHeightmapFile)
{
	// Restriction: RAW file dimensions must be >= to the
	// dimensions of the terrain.  That is a 128x128 RAW file
	// can only be used with a terrain constructed with at most
	// 128x128 vertices.
	std::string strTemp;

	std::ifstream inFile (strHeightmapFile.c_str() /*, std::ios_base::binary*/);

	if ( inFile == 0 )
		return FALSE;

	// setup the terrain
	// read PGM file format tag
	inFile >> strTemp;

	// read row and cols
	inFile >> m_numRow >> m_numCol;

	m_numCellsPerRow = (m_numRow - 1);
	m_numCellsPerCol = (m_numCol - 1);
	m_rWidth =  m_numCellsPerCol * m_rCellSpacing;
	m_rDepth =  m_numCellsPerRow * m_rCellSpacing;
	m_numVertices  = m_numRow * m_numCol;
	m_numTriangles = m_numCellsPerRow * m_numCellsPerCol * 2;

	// read range
	inFile >> strTemp;

	// A height for each vertex
	m_aHeightmap.resize( m_numVertices );

	// load height map 
	for (unsigned int i = 0; i < m_aHeightmap.size(); i++)
	{
		inFile >> m_aHeightmap[i];
	}

	inFile.close();

	return true;
}

BOOL CD3DTerrain::loadTexture (const std::string &strTextureFile)
{
	if (FAILED (D3DXCreateTextureFromFile (m_pD3DDevice, strTextureFile.c_str(), &m_pTexture)))
		return FALSE;

	return TRUE;
}

BOOL CD3DTerrain::computeVertices ()
{
	if (FAILED (m_pD3DDevice->CreateVertexBuffer (m_numVertices * sizeof(STerrainVertex),
		D3DUSAGE_WRITEONLY, STerrainVertex::FVF, D3DPOOL_MANAGED, &m_pVertexBuff, 0)))
		return FALSE;

	STerrainVertex* aVertexBuff = 0;
	m_pVertexBuff->Lock(0, 0, (void**)&aVertexBuff, 0);

	// coordinates to start generating vertices at
	float startX = -m_rWidth / 2.0f;
	float startZ = -m_rDepth / 2.0f;

	// coordinates to end generating vertices at
	float endX = m_rWidth / 2.0f;
	float endZ = m_rDepth / 2.0f;

	// compute the increment size of the texture coordinates
	// from one vertex to the next.
	float uCoordIncrementSize = 1.0f / (float)m_numCellsPerCol;
	float vCoordIncrementSize = 1.0f / (float)m_numCellsPerRow;

	int i = 0;
	for(float z = startZ; z <= endZ; z += m_rCellSpacing, i++)
	{
		int j = 0;
		for(float x = startX; x <= endX; x += m_rCellSpacing, j++)
		{
			// compute the correct index into the vertex buffer and heightmap
			// based on where we are in the nested loop.
			int index = i * m_numCol + j;

			aVertexBuff[index] = STerrainVertex(x, m_aHeightmap[index], z,
				(float)j * uCoordIncrementSize,	(float)i * vCoordIncrementSize);
		}
	}

	m_pVertexBuff->Unlock();

	return TRUE;
}

BOOL CD3DTerrain::computeIndices ()
{
	if (FAILED (m_pD3DDevice->CreateIndexBuffer (m_numTriangles * 3 * sizeof(WORD),
		D3DUSAGE_WRITEONLY,	D3DFMT_INDEX16,	D3DPOOL_MANAGED, &m_pIndexBuff, 0)))
		return FALSE;

	WORD* aIndexBuff = 0;
	m_pIndexBuff->Lock(0, 0, (void**)&aIndexBuff, 0);

	// index to start of a group of 6 indices that describe the
	// two triangles that make up a quad
	int baseIndex = 0;

	// loop through and compute the triangles of each quad
	for(int i = 0; i < m_numCellsPerRow; i++)
	{
		for(int j = 0; j < m_numCellsPerCol; j++)
		{
			aIndexBuff[baseIndex++] =    i    * m_numCol + j;
			aIndexBuff[baseIndex++] =    i    * m_numCol + j + 1;
			aIndexBuff[baseIndex++] = (i + 1) * m_numCol + j;

			aIndexBuff[baseIndex++] = (i + 1) * m_numCol + j;
			aIndexBuff[baseIndex++] =    i    * m_numCol + j + 1;
			aIndexBuff[baseIndex++] = (i + 1) * m_numCol + j + 1;
		}
	}

	m_pIndexBuff->Unlock();

	return true;
}

float CD3DTerrain::getHeightmapEntry (int row, int col)
{
	if (row >= 0 && row < m_numRow && col >= 0 && col < m_numCol)
		return m_aHeightmap[row * m_numRow + col];
	else
		return 0.0;
}

void CD3DTerrain::setHeightmapEntry (int row, int col, float value)
{
	m_aHeightmap[row * m_numRow + col] = value;
}

float CD3DTerrain::getHeight (float x, float z)
{
	if (NULL == m_pD3DDevice)
		return 0.0;

	// Translate on xz-plane by the transformation that takes
	// the terrain START point to the origin.
	x = (m_rWidth / 2.0f) + x;
	z = (m_rDepth / 2.0f) - z;

	// Scale down by the transformation that makes the 
	// cellspacing equal to one.  This is given by 
	// 1 / cellspacing since; cellspacing * 1 / cellspacing = 1.
	x /= m_rCellSpacing;
	z /= m_rCellSpacing;

	// From now on, we will interpret our positive z-axis as
	// going in the 'down' direction, rather than the 'up' direction.
	// This allows to extract the row and column simply by 'flooring'
	// x and z:
	int col = (int)floorf(x);
	int row = (int)floorf(z);

	// get the heights of the quad we're in:
	// 
	//  A   B
	//  *---*
	//  | / |
	//  *---*  
	//  C   D
	float A = getHeightmapEntry(row,   col);
	float B = getHeightmapEntry(row,   col + 1);
	float C = getHeightmapEntry(row + 1, col);
	float D = getHeightmapEntry(row + 1, col + 1);

	//
	// Find the triangle we are in:
	//

	// Translate by the transformation that takes the upper-left
	// corner of the cell we are in to the origin.  Recall that our 
	// cellspacing was nomalized to 1.  Thus we have a unit square
	// at the origin of our +x -> 'right' and +z -> 'down' system.
	float dx = x - col;
	float dz = z - row;

	// Note the below compuations of u and v are unneccessary, we really
	// only need the height, but we compute the entire vector to emphasis
	// the books discussion.
	float height = 0.0f;
	if(dz < 1.0f - dx)  // upper triangle ABC
	{
		float uy = B - A; // A->B
		float vy = C - A; // A->C

		// Linearly interpolate on each vector.  The height is the vertex
		// height the vectors u and v originate from {A}, plus the heights
		// found by interpolating on each vector u and v.
		height = A + linearInterpolate (0.0f, uy, dx) + linearInterpolate (0.0f, vy, dz);
	}
	else // lower triangle DCB
	{
		float uy = C - D; // D->C
		float vy = B - D; // D->B

		// Linearly interpolate on each vector.  The height is the vertex
		// height the vectors u and v originate from {D}, plus the heights
		// found by interpolating on each vector u and v.
		height = D + linearInterpolate (0.0f, uy, 1.0f - dx) + linearInterpolate (0.0f, vy, 1.0f - dz);
	}

	return height;
}
