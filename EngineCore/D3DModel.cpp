#include <iostream>
#include <algorithm>
#include <string>

#include "D3DModel.h"
#include "D3DObjMeshLoader.h"

using namespace std;

const DWORD CD3DModel::SVertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

CD3DModel::CD3DModel(void)
{
	// initialize variables
	m_pD3DDevice = NULL;
	m_pObjectMesh = NULL;
	m_apTexture.clear();
	m_asMaterial.clear();

	// initialize world matrix
	D3DXMatrixTranslation(&m_matWorld,  0.0f, 0.0f,  0.0f);
}

CD3DModel::~CD3DModel(void)
{
	unload3DModel ();
}

BOOL CD3DModel::load3DModel (IDirect3DDevice9* pD3DDevice, const std::string &strModelFile, float rScale)
{
	// set device and model parameters
	m_pD3DDevice = pD3DDevice;

	unsigned uExtPos = strModelFile.find_last_of (".");
	std::string strExtension = strModelFile.substr (uExtPos + 1);
	std::transform (strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);

	if(uExtPos != std::string::npos && strExtension == "obj")
	{
		// Create the object.
		CD3DObjMeshLoader	cObjMeshLoader;
		HRESULT hr = cObjMeshLoader.Create (strModelFile, pD3DDevice, &m_pObjectMesh, rScale);

		// create material and texture
		unsigned int nNumSubsets = cObjMeshLoader.GetNumSubsets ();
		m_asMaterial.resize (nNumSubsets);
		m_apTexture.resize (nNumSubsets);
		for (unsigned int index = 0; index < nNumSubsets; index++)
		{
			SMaterial sMaterial;
			cObjMeshLoader.GetSubsetMaterial (sMaterial, index);
			if (sMaterial.vAmbient == D3DXVECTOR3 (0.0, 0.0,0.0))
			{
				sMaterial.vAmbient = sMaterial.vDiffuse;
			}
			m_asMaterial[index] = initMtrl (vectorToColor (sMaterial.vAmbient), 
				vectorToColor (sMaterial.vDiffuse), 
				vectorToColor (sMaterial.vSpecular), vectorToColor (sMaterial.vEmissive), 2.0f);
			if ( sMaterial.strTexture != "" )
			{            
				if (FAILED (D3DXCreateTextureFromFile (pD3DDevice, 
					sMaterial.strTexture.c_str(),
					&m_apTexture[index])))
				{
					return FALSE;
				}
#ifdef _DEBUG
				cout << "Loaded texture file: " << sMaterial.strTexture << endl;
#endif
			}
			else
			{
				m_apTexture[index] = NULL;
			}
		}

		// compute normals 
		//D3DXComputeNormals (m_pObjectMesh, NULL);
	}
	else
	{
#ifdef _DEBUG
		cout << "Unsupported file format: " << strExtension << endl;
#endif
		return FALSE;
	}

	return  TRUE;
}

void CD3DModel::setWorldTransform (const D3DXMATRIX &matWorld)
{
	// set world matrix - position the object in world space
	m_matWorld = matWorld;
}

// preform model rendering
BOOL CD3DModel::renderModel ()
{
	if (m_pD3DDevice)
	{
		// Set the world matrix that positions the object.
		m_pD3DDevice->SetTransform (D3DTS_WORLD, &m_matWorld);

		for (unsigned int index = 0; index < m_asMaterial.size(); index++)
		{
			// Set object material
			m_pD3DDevice->SetMaterial (&m_asMaterial[index]);

			// Set object texture
			m_pD3DDevice->SetTexture (0, m_apTexture[index]);

			// Draw the object using the previously set world matrix.
			if (m_pObjectMesh)
			{
				m_pObjectMesh->DrawSubset (index + 1);
			}
		}
	}

	return  TRUE;
}

BOOL CD3DModel::unload3DModel ()
{
	m_asMaterial.clear ();

	for (unsigned int index = 0; index < m_apTexture.size(); index++)
	{
		_SAFE_RELEASE (m_apTexture[index]);
	}
	m_apTexture.clear ();

	_SAFE_RELEASE (m_pObjectMesh);

	return  TRUE;
}
