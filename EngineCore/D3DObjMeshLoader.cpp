#include <iostream>
#include <fstream>
#include <sstream>
#include "D3DObjMeshLoader.h"
#include "D3DModel.h"

using namespace std;

//--------------------------------------------------------------------------------------
CD3DObjMeshLoader::CD3DObjMeshLoader()
{
	//m_pd3dDevice = NULL;
	//m_pMesh = NULL;
	//m_strMediaDir = "";

	m_dwAttribTableEntries = 0;
	m_aAttribTable = NULL;

	m_adjacencyInfo = NULL;
	m_optimizedAdjacencyInfo = NULL;
}

//--------------------------------------------------------------------------------------
CD3DObjMeshLoader::~CD3DObjMeshLoader()
{
	Destroy();
}

//--------------------------------------------------------------------------------------
HRESULT CD3DObjMeshLoader::Create (const std::string &strFilename, IDirect3DDevice9* pd3dDevice, ID3DXMesh** ppMesh, float rScale)
{
	HRESULT hr;

	// Start clean
	Destroy();

	// Store the device pointer
	//m_pd3dDevice = pd3dDevice;

	// Store the directory where the mesh was found
	//m_strMediaDir = ""; //strFilename.find_last_of("\\/");

	// Set the current directory based on where the mesh was found
	//char szOldDir[_MAX_PATH];
	//GetCurrentDirectory (_MAX_PATH, szOldDir );
	//SetCurrentDirectory( m_strMediaDir.c_str() );   

	// Load the vertex buffer, index buffer, and subset information from a file. In this case, 
	// an .obj file was chosen for simplicity, but it's meant to illustrate that ID3DXMesh objects
	// can be filled from any mesh file format once the necessary data is extracted from file.
	hr = LoadGeometryFromOBJ (strFilename);
	if (FAILED (hr))
	{
		return hr;
	}

	// Restore the original current directory
	//SetCurrentDirectory( szOldDir );

	// Create the encapsulated mesh
	ID3DXMesh *pMesh = NULL;

	hr = D3DXCreateMeshFVF (m_aIndices.size() / 3, m_aVertices.size(),
		D3DXMESH_32BIT | D3DXMESH_MANAGED, CD3DModel::SVertex::FVF, pd3dDevice, &pMesh);
	if(FAILED(hr))
	{
		return FALSE;
	}

	// load vertex buffer
	CD3DModel::SVertex	sTempVertex;
	::ZeroMemory (&sTempVertex, sizeof (CD3DModel::SVertex));
	CD3DModel::SVertex *arVertexBuff = NULL;
	pMesh->LockVertexBuffer(0, (void**)&arVertexBuff);
	for (unsigned int i = 0; i < m_aVertices.size(); i++)
	{
		sTempVertex._x = -rScale * m_aVertices [i].vPosition.x; // D3D uses LH coordinate system
		sTempVertex._y = rScale * m_aVertices [i].vPosition.y;
		sTempVertex._z = rScale * m_aVertices [i].vPosition.z;
		sTempVertex._u = m_aVertices [i].vTexcoord.x;
		sTempVertex._v = 1.0f - m_aVertices [i].vTexcoord.y; // D3D uses LH coordinate system
		sTempVertex._nx = m_aVertices [i].vNormal.x;
		sTempVertex._ny = m_aVertices [i].vNormal.y;
		sTempVertex._nz = m_aVertices [i].vNormal.z;
		arVertexBuff[i] = sTempVertex;
	}
	pMesh->UnlockVertexBuffer();

	// load index buffer
	DWORD* arIndexBuff = NULL;
	pMesh->LockIndexBuffer(0, (void**)&arIndexBuff);
	for (unsigned int i = 0; i < m_aIndices.size(); i++)
	{
		arIndexBuff[i] = m_aIndices[i];
	}
	pMesh->UnlockIndexBuffer();

	// load attribute buffer
	DWORD* arAttribBuff = 0;
	pMesh->LockAttributeBuffer(0, &arAttribBuff);
	for (unsigned int i = 0; i < m_aAttributes.size(); i++)
	{
		arAttribBuff [i] = m_aAttributes[i];
	}
	pMesh->UnlockAttributeBuffer();

	// Get the adjacency info of the non-optimized mesh.
	m_adjacencyInfo = new DWORD [pMesh->GetNumFaces() * 3];
	pMesh->GenerateAdjacency(0.0f, m_adjacencyInfo);

	// Array to hold optimized adjacency info.
	m_optimizedAdjacencyInfo = new DWORD[pMesh->GetNumFaces() * 3];

	// Reorder the vertices according to subset and optimize the mesh for this graphics 
	// card's vertex cache. When rendering the mesh's triangle list the vertices will 
	// cache hit more often so it won't have to re-execute the vertex shader.
	pMesh->GenerateAdjacency( 0.0f, NULL );
	pMesh->OptimizeInplace (D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_COMPACT | D3DXMESHOPT_VERTEXCACHE,
		m_adjacencyInfo, m_optimizedAdjacencyInfo, NULL, NULL);

	// compute the normals:
	//D3DXComputeNormals (pMesh, NULL /*m_optimizedAdjacencyInfo*/);

	pMesh->GetAttributeTable( NULL, &m_dwAttribTableEntries );
	m_aAttribTable = new D3DXATTRIBUTERANGE[m_dwAttribTableEntries];
	pMesh->GetAttributeTable( m_aAttribTable, &m_dwAttribTableEntries );

	*ppMesh = pMesh;

	return S_OK;
}

//--------------------------------------------------------------------------------------
void CD3DObjMeshLoader::Destroy()
{
	//m_strMediaDir = "";	

	m_aVertices.clear();
	m_aIndices.clear();
	m_aAttributes.clear();
	m_aMaterials.clear();

	if ( m_aAttribTable )
	{
		m_dwAttribTableEntries = 0;
		delete[] m_aAttribTable;
		m_aAttribTable = NULL;
	}

	if ( m_adjacencyInfo )
	{
		delete[] m_adjacencyInfo;
		m_adjacencyInfo = NULL;
	}

	if ( m_optimizedAdjacencyInfo )
	{
		delete[] m_optimizedAdjacencyInfo;
		m_optimizedAdjacencyInfo = NULL;
	}

	//if( m_pMesh )
	//{
	//	m_pMesh->Release ();
	//}

	//m_pd3dDevice = NULL;
}

//--------------------------------------------------------------------------------------
HRESULT CD3DObjMeshLoader::LoadGeometryFromOBJ (const std::string &strFilename)
{
	HRESULT hr;
	char szMtlFilename [_MAX_PATH];

	// Create temporary storage for the input data. Once the data has been loaded into
	// a reasonable format we can create a D3DXMesh object and load it with the mesh data.
	std::vector <D3DXVECTOR3> avPositions;
	std::vector <D3DXVECTOR2> avTexCoords;
	std::vector <D3DXVECTOR3> avNormals;

	// The first subset uses the default material
	SMaterial sMaterial;
	InitMaterial (sMaterial);
	m_aMaterials.push_back (sMaterial);

	// File input
	std::ifstream InFile (strFilename);
	if (!InFile)
		return S_FALSE;

#ifdef _DEBUG
	cout << "Loading mesh file... " << strFilename << endl;
#endif

	DWORD dwCurSubset = 0;
	char szLineBuff [_MAX_PATH];
	char szCommand [_MAX_PATH];
	while (1)
	{
		InFile.getline (szLineBuff, _MAX_PATH);
		if( !InFile )
			break;

		szCommand [0] = 0;
		sscanf (szLineBuff, "%s", szCommand);

		if( 0 == strcmp( szCommand, "#" ) )
		{
			// Comment
		}
		else if( 0 == strcmp( szCommand, "v" ) )
		{
			// Vertex Position
			float x = 0.0f, y = 0.0f, z = 0.0f;
			sscanf (szLineBuff, "%s%f%f%f", szCommand, &x, &y, &z);
			avPositions.push_back( D3DXVECTOR3( x, y, z ) );
		}
		else if( 0 == strcmp( szCommand, "vt" ) )
		{
			// Vertex TexCoord
			float u = 0.0f, v = 0.0f;
			sscanf (szLineBuff, "%s%f%f", szCommand, &u, &v);
			avTexCoords.push_back( D3DXVECTOR2( u, v ) );
		}
		else if( 0 == strcmp( szCommand, "vn" ) )
		{
			// Vertex Normal
			float x = 0.0f, y = 0.0f, z = 0.0f;
			sscanf (szLineBuff, "%s%f%f%f", szCommand, &x, &y, &z);
			avNormals.push_back( D3DXVECTOR3( x, y, z ) );
		}
		else if( 0 == strcmp( szCommand, "f" ) )
		{
			// detecting face format
			int iStrPos = 0;
			int cNumSlashes = 0;
			int iSlashPos = -1;
			bool fPairedSlashes = false;
			while (0 != szLineBuff[iStrPos])
			{
				if ('/' == szLineBuff[iStrPos])
					cNumSlashes++;

				iStrPos++;
			}

			if (cNumSlashes > 0)
			{
				iSlashPos = strchr (szLineBuff, '/') - szLineBuff;
				if ('/' == szLineBuff[iSlashPos + 1])
					fPairedSlashes = true;
			}

			// Face
			D3DXVERTEX aVertex[3];
			int iPosition[3], iTexCoord[3], iNormal[3];

			::ZeroMemory( aVertex, sizeof( D3DXVERTEX ) * 3 );
			::ZeroMemory( iPosition, sizeof( int ) * 3 );
			::ZeroMemory( iTexCoord, sizeof( int ) * 3 );
			::ZeroMemory( iNormal, sizeof( int ) * 3 );
			// read face indices, obj file vertex indices starts from 1. Need to reassign to 0.
			if (0 == cNumSlashes) // (0 == avTexCoords.size() && 0 == avNormals.size()) // only vertex indices
			{
				sscanf (szLineBuff, "%s%d%d%d", szCommand, 
					&iPosition[0], &iPosition[1], &iPosition[2]);
			}
			else if (3 == cNumSlashes) // (0 != avTexCoords.size() && 0 == avNormals.size()) // vertex and texture indices
			{
				sscanf (szLineBuff, "%s%d/%d%d/%d%d/%d", szCommand, 
					&iPosition[0], &iTexCoord[0], 
					&iPosition[1], &iTexCoord[1], 
					&iPosition[2], &iTexCoord[2]);
			}
			else if (6 == cNumSlashes && true == fPairedSlashes) // (0 == avTexCoords.size() && 0 != avNormals.size()) // vertex and normal indices
			{
				sscanf (szLineBuff, "%s%d//%d%d//%d%d//%d", szCommand, 
					&iPosition[0], &iNormal[0], 
					&iPosition[1], &iNormal[1], 
					&iPosition[2], &iNormal[2]);
			}
			else if (6 == cNumSlashes && false == fPairedSlashes) // all three indices
			{
				sscanf (szLineBuff, "%s%d/%d/%d%d/%d/%d%d/%d/%d", szCommand, 
					&iPosition[0], &iTexCoord[0], &iNormal[0], 
					&iPosition[1], &iTexCoord[1], &iNormal[1], 
					&iPosition[2], &iTexCoord[2], &iNormal[2]);
			}
			else
			{
				continue;
			}

			for( UINT iFace = 0; iFace < 3; iFace++ )
			{
				// OBJ format uses 1-based arrays
				aVertex[iFace].vPosition = avPositions[ iPosition[iFace] - 1 ];
				if (cNumSlashes > 0 && false == fPairedSlashes) // (0 != avTexCoords.size())
					aVertex[iFace].vTexcoord = avTexCoords[ iTexCoord[iFace] - 1 ];
				if (6 == cNumSlashes) // (0 != avNormals.size())
					aVertex[iFace].vNormal = avNormals[ iNormal[iFace] - 1 ];
				// If a duplicate vertex doesn't exist, add this vertex to the Vertices
				// list. Store the index in the Indices array. The Vertices and Indices
				// lists will eventually become the Vertex Buffer and Index Buffer for
				// the mesh.
				DWORD index = AddVertex ( iPosition[iFace], &aVertex[iFace] );
				m_aIndices.push_back ( index );
			}
			m_aAttributes.push_back( dwCurSubset );
		}
		else if( 0 == strcmp( szCommand, "mtllib" ) )
		{
			// Material library
			szMtlFilename [0] = 0;
			sscanf (szLineBuff, "%s%s", szCommand, szMtlFilename);
		}
		else if( 0 == strcmp( szCommand, "usemtl" ) )
		{
			// Material
			char szMtlName [_MAX_PATH];
			szMtlName [0] = 0;
			sscanf (szLineBuff, "%s%s", szCommand, szMtlName);

			bool bFound = false;
			for( unsigned int iMaterial = 0; iMaterial < m_aMaterials.size(); iMaterial++ )
			{
				if( 0 == strcmp( m_aMaterials[iMaterial].strName.c_str(), szMtlName ) )
				{
					bFound = true;
					dwCurSubset = iMaterial;
					break;
				}
			}

			if( !bFound )
			{
				SMaterial sMaterial;
				InitMaterial (sMaterial);
				m_aMaterials.push_back (sMaterial);

				dwCurSubset = m_aMaterials.size() - 1;
				m_aMaterials[dwCurSubset].strName = szMtlName;
			}
		}
		else
		{
			// unimplemented or unrecognized command
		}
	}

	// Cleanup
	InFile.close();
	DeleteCache();

#ifdef _DEBUG
	cout << "Loaded mesh file: " << strFilename << endl;
#endif

	// If an associated material file was found, read that in as well.
	if( szMtlFilename[0] )
	{
		hr = LoadMaterialsFromMTL( szMtlFilename );
		if (FAILED (hr))
		{
			return hr;
		}
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT CD3DObjMeshLoader::LoadMaterialsFromMTL (const std::string &strFilename)
{
	// File input
	std::ifstream InFile (strFilename);
	if (!InFile)
		return S_FALSE;

#ifdef _DEBUG
	cout << "Loading material file... " << strFilename << endl;
#endif

	unsigned int iMaterial = -1;

	char szLineBuff [_MAX_PATH];
	char szCommand [_MAX_PATH];
	while (1)
	{
		InFile.getline (szLineBuff, _MAX_PATH);
		if( !InFile )
			break;

		szCommand [0] = 0;
		sscanf (szLineBuff, "%s", szCommand);

		if( 0 == strcmp( szCommand, "newmtl" ) )
		{
			// Switching active materials
			char strName [_MAX_PATH];
			strName [0] = 0;
			sscanf (szLineBuff, "%s%s", szCommand, strName);

			for( unsigned int i = 0; i < m_aMaterials.size(); i++ )
			{
				if( 0 == strcmp( m_aMaterials[i].strName.c_str(), strName ) )
				{
					iMaterial = i;
					break;
				}
			}
		}

		// The rest of the commands rely on an active material
		if( -1 == iMaterial )
			continue;

		if( 0 == strcmp( szCommand, "#" ) )
		{
			// Comment
		}
		else if( 0 == strcmp( szCommand, "Ka" ) )
		{
			// Ambient color
			float r = 0.0f, g = 0.0f, b = 0.0f;
			sscanf (szLineBuff, "%s%f%f%f", szCommand, &r, &g, &b);
			m_aMaterials[iMaterial].vAmbient = D3DXVECTOR3( r, g, b );
		}
		else if( 0 == strcmp( szCommand, "Kd" ) )
		{
			// Diffuse color
			float r = 0.0f, g = 0.0f, b = 0.0f;
			sscanf (szLineBuff, "%s%f%f%f", szCommand, &r, &g, &b);
			m_aMaterials[iMaterial].vDiffuse = D3DXVECTOR3( r, g, b );
		}
		else if( 0 == strcmp( szCommand, "Ks" ) )
		{
			// Specular color
			float r = 0.0f, g = 0.0f, b = 0.0f;
			sscanf (szLineBuff, "%s%f%f%f", szCommand, &r, &g, &b);
			m_aMaterials[iMaterial].vSpecular = D3DXVECTOR3( r, g, b );
		}
		else if( 0 == strcmp( szCommand, "Ke" ) )
		{
			// Specular color
			float r = 0.0f, g = 0.0f, b = 0.0f;
			sscanf (szLineBuff, "%s%f%f%f", szCommand, &r, &g, &b);
			m_aMaterials[iMaterial].vEmissive = D3DXVECTOR3( r, g, b );
		}
		else if( 0 == strcmp( szCommand, "d" ) ||
			0 == strcmp( szCommand, "Tr" ) )
		{
			// Alpha
			sscanf (szLineBuff, "%s%f", szCommand, &m_aMaterials[iMaterial].rAlpha);
		}
		else if( 0 == strcmp( szCommand, "Ns" ) )
		{
			// Shininess
			sscanf (szLineBuff, "%s%f", szCommand, &m_aMaterials[iMaterial].rShininess);
		}
		else if( 0 == strcmp( szCommand, "illum" ) )
		{
			// Specular on/off
			int illumination = 2;
			sscanf (szLineBuff, "%s%d", szCommand, &illumination);
			m_aMaterials[iMaterial].bSpecular = true; // ( illumination == 2 );
		}
		else if( 0 == strcmp( szCommand, "map_Kd" ) || 0 == strcmp( szCommand, "map_Ka" ) )
		{
			// Texture
			char szTempBuff [_MAX_PATH];
			szTempBuff [0] = 0;
			sscanf (szLineBuff, "%s%s", szCommand, szTempBuff);
			m_aMaterials[iMaterial].strTexture = szTempBuff;
		}
		else
		{
			// unimplemented or unrecognized command
		}
	}

	InFile.close();

#ifdef _DEBUG
	cout << "Loaded material file: " << strFilename << endl;
#endif

	return S_OK;
}

//--------------------------------------------------------------------------------------
void CD3DObjMeshLoader::InitMaterial (SMaterial &sMaterial)
{
	::ZeroMemory (&sMaterial, sizeof(SMaterial));

	// default material
	sMaterial.strName = "default";
	sMaterial.vAmbient = D3DXVECTOR3( 0.2f, 0.2f, 0.2f );
	sMaterial.vDiffuse = D3DXVECTOR3( 0.8f, 0.8f, 0.8f );
	sMaterial.vSpecular = D3DXVECTOR3( 1.0f, 1.0f, 1.0f );
	sMaterial.rShininess = 0.0f;
	sMaterial.rAlpha = 1.0f;
	sMaterial.bSpecular = true;
	sMaterial.strTexture = "";
}

//--------------------------------------------------------------------------------------
DWORD CD3DObjMeshLoader::AddVertex (UINT uHash, D3DXVERTEX* pVertex)
{
	// If this vertex doesn't already exist in the Vertices list, create a new entry.
	// Add the index of the vertex to the Indices list.
	bool bFoundInList = false;
	UINT uIndex = 0;

	// Since it's very slow to check every element in the vertex list, a hashtable stores
	// vertex indices according to the vertex position's index as reported by the OBJ file
	if( ( UINT )m_aVertexCache.size() > uHash )
	{
		SCacheEntry* pEntry = m_aVertexCache[uHash];
		while( pEntry != NULL )
		{
			D3DXVERTEX* pCacheVertex = &m_aVertices[pEntry->uIndex];

			// If this vertex is identical to the vertex already in the list, simply
			// point the index buffer to the existing vertex
			if( 0 == memcmp( pVertex, pCacheVertex, sizeof( D3DXVERTEX ) ) )
			{
				bFoundInList = true;
				uIndex = pEntry->uIndex;
				break;
			}

			pEntry = pEntry->pNext;
		}
	}

	// Vertex was not found in the list. Create a new entry, both within the Vertices list
	// and also within the hashtable cache
	if( !bFoundInList )
	{
		// Add to the Vertices list
		uIndex = m_aVertices.size();
		m_aVertices.push_back( *pVertex );

		// Add this to the hashtable
		SCacheEntry* pNewEntry = new SCacheEntry;
		if( pNewEntry == NULL )
			return (DWORD)-1;

		pNewEntry->uIndex = uIndex;
		pNewEntry->pNext = NULL;

		// Grow the cache if needed
		while( ( UINT )m_aVertexCache.size() <= uHash )
		{
			m_aVertexCache.push_back( NULL );
		}

		// Add to the end of the linked list
		SCacheEntry* pCurEntry = m_aVertexCache[uHash];
		if( pCurEntry == NULL )
		{
			// This is the head element
			m_aVertexCache[uHash] = pNewEntry;
		}
		else
		{
			// Find the tail
			while( pCurEntry->pNext != NULL )
			{
				pCurEntry = pCurEntry->pNext;
			}

			pCurEntry->pNext = pNewEntry;
		}
	}

	return uIndex;
}

//--------------------------------------------------------------------------------------
void CD3DObjMeshLoader::DeleteCache()
{
	// Iterate through all the elements in the cache and subsequent linked lists
	for( unsigned int i = 0; i < m_aVertexCache.size(); i++ )
	{
		SCacheEntry* pEntry = m_aVertexCache[i];
		while( pEntry != NULL )
		{
			SCacheEntry* pNext = pEntry->pNext;
			delete pEntry;
			pEntry = pNext;
		}
	}

	m_aVertexCache.clear();
}
