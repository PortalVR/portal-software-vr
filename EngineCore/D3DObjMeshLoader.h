#pragma once

#include <string>
#include <vector>
#include "D3DUtility.h"

#define ERROR_RESOURCE_VALUE 1

template<typename TYPE> BOOL IsErrorResource( TYPE data )
{
	if( ( TYPE )ERROR_RESOURCE_VALUE == data )
		return TRUE;
	return FALSE;
}

// Vertex format
typedef struct _tag_D3DXVERTEX
{
	D3DXVECTOR3 vPosition;
	D3DXVECTOR2 vTexcoord;
	D3DXVECTOR3 vNormal;
}D3DXVERTEX;

// Used for a hashtable vertex cache when creating the mesh from a .obj file
typedef struct _tag_SCacheEntry
{
	UINT uIndex;
	_tag_SCacheEntry* pNext;
}SCacheEntry;

// Material properties per mesh subset
typedef struct _tag_SMaterial
{
	std::string		strName;	// new mtl
	D3DXVECTOR3		vAmbient;	// Ka
	D3DXVECTOR3		vDiffuse;	// Kd
	D3DXVECTOR3		vSpecular;	// Ks
	D3DXVECTOR3		vEmissive;	// Ke
	float			rShininess;	// Ns
	float			rAlpha;		// d or Tr
	bool			bSpecular;	// illum
	std::string		strTexture; // map_Ka
}SMaterial;

class CD3DObjMeshLoader
{
public:
	CD3DObjMeshLoader();
	~CD3DObjMeshLoader();

	HRESULT Create (const std::string &strFilename, IDirect3DDevice9* pd3dDevice, ID3DXMesh** ppMesh, float rScale = 1.0f);
	void Destroy();

	UINT GetNumMaterials() const
	{
		return m_aMaterials.size();
	}

	void GetMaterial (SMaterial &sMaterial, UINT iMaterial)
	{
		sMaterial = m_aMaterials [iMaterial];
	}

	UINT GetNumSubsets()
	{
		return m_dwAttribTableEntries;
	}

	void GetSubsetMaterial (SMaterial &sMaterial, UINT iSubset)
	{
		sMaterial = m_aMaterials [m_aAttribTable[iSubset].AttribId];
	}

	//ID3DXMesh* GetMesh()
	//{
	//	return m_pMesh;
	//}

	//void GetMediaDirectory(std::string &strMediaDir)
	//{
	//	strMediaDir = m_strMediaDir;
	//}

private:
	HRESULT LoadGeometryFromOBJ (const std::string &strFilename);
	HRESULT LoadMaterialsFromMTL (const std::string &strFileName);
	void    InitMaterial (SMaterial &sMaterial);

	DWORD   AddVertex (UINT hash, D3DXVERTEX* pVertex);
	void    DeleteCache();

	//IDirect3DDevice9*			m_pd3dDevice;   // Direct3D Device object associated with this mesh
	//ID3DXMesh*				m_pMesh;        // Encapsulated D3DX Mesh
	//std::string				m_strMediaDir;	// Directory where the mesh was found

	std::vector <SCacheEntry*>	m_aVertexCache; // Hashtable cache for locating duplicate vertices
	std::vector <D3DXVERTEX>	m_aVertices;    // Filled and copied to the vertex buffer
	std::vector <DWORD>			m_aIndices;     // Filled and copied to the index buffer
	std::vector <DWORD>			m_aAttributes;	// Filled and copied to the attribute buffer
	std::vector <SMaterial>		m_aMaterials;	// Holds material properties per subset

	DWORD						m_dwAttribTableEntries;
	D3DXATTRIBUTERANGE*			m_aAttribTable;

	DWORD*						m_adjacencyInfo;
	DWORD *						m_optimizedAdjacencyInfo;
};
