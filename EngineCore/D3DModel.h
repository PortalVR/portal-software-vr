#pragma once

#include <string>
#include <vector>
#include "D3DUtility.h"

class CD3DModel
{
public:
	struct SVertex
	{
		SVertex () {}
		SVertex (float x, float y, float z, 
			float nx, float ny, float nz, float u, float v)
		{
			_x = x;   _y = y;   _z = z;
			_nx = nx; _ny = ny; _nz = nz;
			_u = u;   _v = v;
		}

		float _x, _y, _z, _nx, _ny, _nz, _u, _v;

		static const DWORD FVF;
	};

protected:
	IDirect3DDevice9*				m_pD3DDevice;
	D3DXMATRIX						m_matWorld;
	ID3DXMesh*						m_pObjectMesh;
	std::vector<D3DMATERIAL9>		m_asMaterial;
	std::vector<IDirect3DTexture9*>	m_apTexture;

public:
	CD3DModel(void);
	~CD3DModel(void);
	BOOL load3DModel (IDirect3DDevice9* pD3DDevice, const std::string &strModelFile, float rScale = 1.0f);
	void setWorldTransform (const D3DXMATRIX &matWorld);
	BOOL renderModel ();
	BOOL unload3DModel ();
};
