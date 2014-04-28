// D3DRenderer.h - direct 3D renderer class declaration
#pragma once

#include <vector>
#include "D3Dutility.h"
#include "D3DSettings.h"
#include "D3DCamera.h"
#include "D3DTerrain.h"
#include "D3DModel.h"

// CD3DRenderer
class CD3DRenderer
{
public:
	// construction
	CD3DRenderer(CD3DSettings& cSettings);
	virtual ~CD3DRenderer();

protected:
	// implementation
	CD3DSettings&			m_cSettings;
	HWND					m_hRenderWnd;
	BOOL					m_fWindowed;
	UINT					m_nWidth;
	UINT					m_nHeight;

	IDirect3D9*				m_pD3D;
	IDirect3DDevice9*		m_pD3DDevice;
	ID3DXFont*				m_pMsgStringFont;
	D3DXMATRIX				m_persProjection;
	D3DVIEWPORT9			m_asViewPort[2];
	//D3DVIEWPORT9			m_sDefaultViewPort;
	CD3DCamera				m_cCamera;

	int						m_nSelectedViewport;
	bool					m_fCrossEyed;
	float					m_rFOV;
	float					m_rFrustumOffset;
	float					m_rEyeOffset;
	float					m_rHorizOverlap[2];
	float					m_rVertiAlign[2];
	float					m_rBarrelFactor;

	ID3DXEffect*			m_pDistEffect;
	IDirect3DSurface9*		m_pBackBuffer;
	IDirect3DTexture9*		m_pPreWarpBuffer;
	IDirect3DVertexDeclaration9* m_pVertDeclaration;

	void drawWarpedBuffer (int iShader, int iViewPort);

	CD3DTerrain				m_cTerrain;
	std::vector<CD3DModel*>	m_apModel;

public:
	// exposed routines
	void CD3DRenderer::ResetParameters ();
	BOOL initD3D (HWND hWnd, UINT nWidth = 0, UINT nHeight = 0, BOOL fWindowed = FALSE);
	BOOL setupD3D (const std::string &strAssetFile);
	BOOL displayD3D (float fTimeDelta);
	void cleanupD3D ();
	void deInitD3D ();

public:
	// message handler
	void OnKeyDown (WPARAM wParam, LPARAM lParam);
	void AsyncKeyDown (float fTimeDelta);
	void MouseMove (float rMoveX, float rMoveY);
	void MouseSpin (float rSpinX, float rSpinY);
};
