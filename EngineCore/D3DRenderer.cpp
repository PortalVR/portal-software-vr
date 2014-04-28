// D3DRenderer.cpp - direct 3D renderer class definition
#include <iostream>
#include <fstream>
#include "D3DRenderer.h"

using namespace std;

struct VERT
{
	float x, y, z, rhw;
	float tu, tv;       
	const static D3DVERTEXELEMENT9 Decl[4];
};

const D3DVERTEXELEMENT9 VERT::Decl[4] =
{
	{ 0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
	{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
	D3DDECL_END()
};

CD3DRenderer::CD3DRenderer(CD3DSettings& cSettings):m_cSettings (cSettings)
{
	m_hRenderWnd = NULL;
	m_fWindowed = FALSE;
	m_pD3D = NULL;
	m_pD3DDevice = NULL;
	m_pMsgStringFont = NULL;

	m_pDistEffect = NULL;
	m_pBackBuffer = NULL;
	m_pPreWarpBuffer = NULL;
	m_pVertDeclaration = NULL;

	ResetParameters ();
}

CD3DRenderer::~CD3DRenderer()
{
	cleanupD3D();
	deInitD3D();
}

void CD3DRenderer::ResetParameters ()
{
	m_nSelectedViewport = 0;	// -1 for left, +1 for right, 0 for both
	m_fCrossEyed = false;		// whether cross eyed display
	m_rFOV = 0.35f;				// FOV in terms of PI, loaded from scene file
	m_rEyeOffset = 0.07f;		// left-right eye offset
	m_rFrustumOffset = 0.0f;	// perspective frustum (related to fov)
	m_rHorizOverlap[0] = 0.5f;	// overlap of left viewport
	m_rHorizOverlap[1] = 0.5f;	// overlap of right viewport
	m_rVertiAlign[0] = 0.0f;	// vertical mis-alignment of left viewport
	m_rVertiAlign[1] = 0.0f;	// vertical mis-alignment of right viewport
	m_rBarrelFactor = -8.0f;	// distortion correction negative = barrel, positive = pincushion
}

// initialize D3D rendering device
BOOL CD3DRenderer::initD3D(HWND hWnd, UINT nWidth, UINT nHeight, BOOL fWindowed)
{
	HRESULT hr = 0;
	m_hRenderWnd = hWnd;
	m_fWindowed = fWindowed;

	// Step 1: Create the IDirect3D9 object.
	if (!(m_pD3D = Direct3DCreate9 (D3D_SDK_VERSION)))
	{
		::MessageBox (hWnd, "Failed to initialize Direct 3D.", "Error", MB_ICONEXCLAMATION);
		return FALSE;
	}
#ifdef _DEBUG
	cout << "Created Direct 3D object." << endl;
#endif

	// Step 2: Check for device capabilities

	// find number of display adapters
	UINT uNumAdapters = m_pD3D->GetAdapterCount ();
#ifdef _DEBUG
	cout << "Number of Display Adapters: " << uNumAdapters << endl << endl;
#endif

	// select a display adapter
	UINT uSelectedAdapter = D3DADAPTER_DEFAULT;
#ifdef _DEBUG
	cout << "Selected Display Adapter ID: " << uSelectedAdapter << endl;
#endif

	// get selected adapter identifier
	D3DADAPTER_IDENTIFIER9 adapterIdentifier;
	if ( FAILED (m_pD3D->GetAdapterIdentifier (uSelectedAdapter, NULL, &adapterIdentifier)))
	{
		::MessageBox (hWnd, "Failed to get adapter identifier.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}
#ifdef _DEBUG
	cout << "Selected Display Adapter Name: " << adapterIdentifier.DeviceName << endl << endl;
#endif

	// get selected adapter monitor
	HMONITOR hMonitor = m_pD3D->GetAdapterMonitor (uSelectedAdapter);

	// get selected adapter mode
	D3DDISPLAYMODE displayMode;
	if( FAILED (m_pD3D->GetAdapterDisplayMode (uSelectedAdapter, &displayMode)))
	{
		::MessageBox (hWnd, "Failed to get display adapter modes.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

	// get number of display modes
	UINT uNumAdapterModes = m_pD3D->GetAdapterModeCount (uSelectedAdapter, displayMode.Format);
#ifdef _DEBUG
	cout << "Available display modes: " << uNumAdapterModes << endl;
	for (UINT iMode = 0; iMode < uNumAdapterModes; iMode++)
	{
		D3DDISPLAYMODE tempDisplayMode;
		if ( FAILED (m_pD3D->EnumAdapterModes (uSelectedAdapter, displayMode.Format, iMode, &tempDisplayMode)))
		{
			::MessageBox (hWnd, "Failed display mode enumerator.", "Error", MB_ICONEXCLAMATION);
			goto END_INIT;
		}
		cout << "Display Mode (" << iMode << "): " << tempDisplayMode.Width << "X" << 
			tempDisplayMode.Height << " " << tempDisplayMode.RefreshRate << "Hz " << endl;
	}
#endif

#ifdef _DEBUG
	cout << endl << "Selected Display Adapter Mode: " << displayMode.Width << "X" << 
		displayMode.Height << " " << displayMode.RefreshRate << "Hz " << endl;
	cout << "Display Mode Format: " << displayMode.Format << endl;
#endif

	// check device type
	int iDeviceType;
	D3DDEVTYPE deviceTypes[] = {D3DDEVTYPE_HAL, D3DDEVTYPE_REF, D3DDEVTYPE_SW, D3DDEVTYPE_NULLREF, (D3DDEVTYPE) 0};
	for (iDeviceType = 0; deviceTypes[iDeviceType] != 0; iDeviceType++)
	{
		if ( SUCCEEDED (m_pD3D->CheckDeviceType (uSelectedAdapter, deviceTypes[iDeviceType],
			displayMode.Format, displayMode.Format, m_fWindowed)))
		{
			break;
		}
	}

	if (0 == deviceTypes[iDeviceType])
	{
		::MessageBox (hWnd, "Failed device type test.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

#ifdef _DEBUG
	cout << "Rendering Device Type: " << deviceTypes[iDeviceType] << endl;
#endif

	// check device depth/stencil buffer format
	int iStencilFormat;
	D3DFORMAT DS32SFormats[] = {D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D24X8, D3DFMT_D32, (D3DFORMAT) NULL};
	for (iStencilFormat = 0; DS32SFormats[iStencilFormat] != 0; iStencilFormat++)
	{
		if ( SUCCEEDED (m_pD3D->CheckDeviceFormat (uSelectedAdapter, deviceTypes[iDeviceType], 
			displayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DS32SFormats[iStencilFormat])))
		{
			if ( SUCCEEDED (m_pD3D->CheckDepthStencilMatch (uSelectedAdapter, deviceTypes[iDeviceType],
				displayMode.Format, displayMode.Format, DS32SFormats[iStencilFormat])))
			{
				break;
			}
		}
	}

	if (NULL == DS32SFormats[iStencilFormat]) // failure
	{
		::MessageBox (hWnd, "Failed device depth/stencil buffer format test.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}
#ifdef _DEBUG
	cout << "Depth/Stencil Format: " << DS32SFormats[iStencilFormat] << endl;
#endif

	// get selected adapter capabilities
	D3DCAPS9 deviceCaps;
	if ( FAILED (m_pD3D->GetDeviceCaps (uSelectedAdapter, deviceTypes[iDeviceType], &deviceCaps)))
	{
		::MessageBox (hWnd, "Failed to get display adapter capabilities.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

	// select between HW/SW vertex processor
	DWORD dwVertexProcessor;	
	if ( deviceCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		dwVertexProcessor = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		dwVertexProcessor = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

#ifdef _DEBUG
	cout << "Vertex Processor: " << dwVertexProcessor << endl;
#endif

	// test antialiasing filter support level
	DWORD dwSampleQualityLevel;
	if( FAILED (m_pD3D->CheckDeviceMultiSampleType (
		uSelectedAdapter, 
		deviceTypes[iDeviceType], 
		displayMode.Format, 
		m_fWindowed, 
		D3DMULTISAMPLE_4_SAMPLES, 
		&dwSampleQualityLevel)))
	{
		::MessageBox (hWnd, "Failed to get antialiasing modes.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

#ifdef _DEBUG
	cout << "Antialiasing filter levels: " << dwSampleQualityLevel << endl;
#endif

	// check if the device’s supported version is less than pixel shader version 3.0
	if( deviceCaps.PixelShaderVersion < D3DPS_VERSION(3, 0) )
	{
		::MessageBox (hWnd, "Requires DirectX 9.0c Shader Model 3.0 or higher.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

	// Step 3: Fill out the D3DPRESENT_PARAMETERS structure.
	m_nWidth = (TRUE == fWindowed)?nWidth:displayMode.Width;
	m_nHeight = (TRUE == fWindowed)?nHeight:displayMode.Height;

#ifdef _DEBUG
	cout << "Backbuffer Size: " << m_nWidth << "X" << m_nHeight << endl << endl;
#endif

	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth            = m_nWidth;
	d3dpp.BackBufferHeight           = m_nHeight;
	d3dpp.BackBufferFormat           = displayMode.Format;
	d3dpp.BackBufferCount            = 1;
	d3dpp.MultiSampleType            = D3DMULTISAMPLE_4_SAMPLES;
	d3dpp.MultiSampleQuality         = dwSampleQualityLevel - 1;
	d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD; 
	d3dpp.hDeviceWindow              = m_hRenderWnd;
	d3dpp.Windowed                   = m_fWindowed;
	d3dpp.EnableAutoDepthStencil     = TRUE; 
	d3dpp.AutoDepthStencilFormat     = DS32SFormats[iStencilFormat];
	d3dpp.Flags                      = 0;
	d3dpp.FullScreen_RefreshRateInHz = (m_fWindowed)?D3DPRESENT_RATE_DEFAULT:displayMode.RefreshRate;
	d3dpp.PresentationInterval       = (m_fWindowed)?D3DPRESENT_INTERVAL_IMMEDIATE:D3DPRESENT_INTERVAL_IMMEDIATE;

	// Step 4: Create the device.
	hr = m_pD3D->CreateDevice(
		uSelectedAdapter,			// primary adapter
		deviceTypes[iDeviceType],	// device type
		m_hRenderWnd,				// window associated with device
		dwVertexProcessor,			// vertex processing
		&d3dpp,						// present parameters
		&m_pD3DDevice);				// return created device

	if( FAILED(hr) )
	{
#ifdef _DEBUG
		cout << "CreateDevice Result: " << hr << endl << endl;
#endif
		m_pD3DDevice = NULL;
		::MessageBox (hWnd, "Failed to create Direct 3D device.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

	// Step 5: Create distortion effect
	ID3DXBuffer* pCompilerError;
	if ( FAILED (D3DXCreateEffectFromFile (m_pD3DDevice, "distortion.fx", NULL, NULL, NULL, NULL, &m_pDistEffect, &pCompilerError)))
	{
		::MessageBox (hWnd, (char *)pCompilerError->GetBufferPointer(), "Error", MB_ICONEXCLAMATION);
		pCompilerError->Release();
		goto END_INIT;
	}
	m_pD3DDevice->GetBackBuffer (0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
	m_pD3DDevice->CreateTexture (m_nWidth, m_nHeight, 1, D3DUSAGE_RENDERTARGET, displayMode.Format, D3DPOOL_DEFAULT, &m_pPreWarpBuffer, NULL);
	m_pD3DDevice->CreateVertexDeclaration (VERT::Decl, &m_pVertDeclaration);

	m_pMsgStringFont = initFont (m_pD3DDevice);
	if (NULL == m_pMsgStringFont)
	{
		::MessageBox (hWnd, "Failed to initialize fonts.", "Error", MB_ICONEXCLAMATION);
		goto END_INIT;
	}

	return TRUE;

END_INIT:
	_SAFE_RELEASE (m_pD3D); // done with d3d9 object
	return FALSE;
}

// setup scene
BOOL CD3DRenderer::setupD3D (const std::string &strAssetFile)
{
	if( m_pD3DDevice ) // Only use Device methods if we have a valid device.
	{
		// Store the directory where the mesh was found
		char szMediaDir[_MAX_PATH];
		int index = strAssetFile.find_last_of("\\/");
		if (index > 0)
		{
			strcpy (szMediaDir, strAssetFile.c_str());
			szMediaDir [index] = 0;
		}
		else
		{
			szMediaDir [0] = 0;
		}

		// Set the current directory based on where the mesh was found
		char szOldDir[_MAX_PATH];
		GetCurrentDirectory (_MAX_PATH, szOldDir );
		SetCurrentDirectory ( szMediaDir );   
#ifdef _DEBUG
		cout << "Current directory: " << endl << szOldDir << endl;
		cout << "Asset directory: " << endl << szMediaDir << endl;
#endif
		// File input
		std::ifstream InFile (strAssetFile.c_str() + index + 1);
		if (!InFile)
			return FALSE;
#ifdef _DEBUG
		cout << "Loading scene file... " << strAssetFile.c_str() + index + 1 << endl;
#endif
		int nLightCount = 0;
		int	nObjectCount = 0;
		char szLineBuff [_MAX_PATH];
		char szCommand [_MAX_PATH];
		while (1)
		{
			InFile.getline (szLineBuff, _MAX_PATH);
			if( !InFile )
				break;

			sscanf (szLineBuff, "%s", szCommand);

			if ( 0 == strcmp( szCommand, "#" ) )
			{
				// Comment
			}
			else if ( 0 == strcmp( szCommand, "camera" ) )
			{
				int nCamType;
				float x, y, z;
				float lx, ly, lz;
				float height;
				sscanf (szLineBuff, "%s%d%f%f%f%f%f%f%f%f", szCommand, &nCamType, &x, &y, &z,
					&lx, &ly, &lz, &height, &m_rFOV);

				// Set the projection matrix.
				D3DXMatrixPerspectiveFovLH(
					&m_persProjection,
					D3DX_PI * m_rFOV, // field of view
					(float)m_nWidth / (float)m_nHeight,
					0.1f,
					1000.0f);
				m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &m_persProjection);

				// setup camera viewpoint
				ECameraType eCameraType = (0 == nCamType)?LANDOBJECT:AIRCRAFT;
				D3DXVECTOR3	camPos (x, y, z);
				D3DXVECTOR3	camLook (lx, ly, lz);
				m_cCamera.setCameraType (eCameraType);
				m_cCamera.setPosition (camPos);
				m_cCamera.setLook (camLook);
				m_cCamera.SetCameraHeight (height);

				for (int iViewPort = 0; iViewPort < 2; iViewPort++)
				{
					m_asViewPort[iViewPort].X      = iViewPort * m_nWidth / 2;
					m_asViewPort[iViewPort].Y      = 0;
					m_asViewPort[iViewPort].Width  = m_nWidth / 2;
					m_asViewPort[iViewPort].Height = m_nHeight;
					m_asViewPort[iViewPort].MinZ   = 0.0f;
					m_asViewPort[iViewPort].MaxZ   = 1.0f;
				}

				//m_pD3DDevice->GetViewport (&m_sDefaultViewPort);
#ifdef _DEBUG
				cout << "Initialized viewport camera." << endl;
#endif
			}
			else if ( 0 == strcmp( szCommand, "viewport" ) )
			{
				sscanf (szLineBuff, "%s%f%f%f%f%f%f%f", szCommand, &m_rFrustumOffset, &m_rEyeOffset,
					&m_rHorizOverlap[0], &m_rHorizOverlap[1], &m_rVertiAlign[0], &m_rVertiAlign[1], &m_rBarrelFactor);
			}
			else if ( 0 == strcmp( szCommand, "light" ) )
			{
				// setup the light sources
				nLightCount++;

				// directional light
				D3DXVECTOR3 dir(0.0f, -1.0f, 0.0f);
				D3DXCOLOR lightColor (D3DCOLOR_XRGB (224, 224, 224));
				D3DLIGHT9 light = initDirectionalLight(dir, lightColor);
				m_pD3DDevice->SetLight(0, &light);
				m_pD3DDevice->LightEnable(0, TRUE);

				// ambient light
				m_pD3DDevice->SetRenderState(D3DRS_AMBIENT, lightColor * 0.6f);

				// set rendering parameters
				m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);	// cull no triangles
				m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE); 		// switch on light
				//m_pD3DDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT); // flat shading within each face
				//m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME); // render wire frame
				m_pD3DDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);	// enable specular reflection
				m_pD3DDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE); // enable scale independent lighting

				// set texture filters
				m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);	// D3DTEXF_LINEAR
				m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);	// D3DTEXF_LINEAR
				m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);			// D3DTEXF_POINT
#ifdef _DEBUG
				cout << "Initialized light sources and rendering state." << endl;
#endif
			}
			else if ( 0 == strcmp( szCommand, "terrain" ) )
			{
				float cs, hs;
				char szHeightMap[_MAX_PATH], szTerrainFile[_MAX_PATH];
				sscanf (szLineBuff, "%s%s%s%f%f", szCommand, szHeightMap, szTerrainFile, &cs, &hs);
#ifdef _DEBUG
				cout << "Loading terrain..." << endl;
#endif			
				// setup the terrain
				if (FALSE == m_cTerrain.loadTerrain (m_pD3DDevice, szHeightMap, szTerrainFile, cs, hs))
				{
					return FALSE;
				}

				// build terrain world matrix
				D3DXMATRIX matWorld;
				D3DXMatrixTranslation(&matWorld,  0.0f, 0.0f,  0.0f);
				m_cTerrain.setWorldTransform (matWorld);
			}
			else if ( 0 == strcmp( szCommand, "object" ) )
			{
				float scale, yaw, pitch, roll, tx, ty, tz;
				char szObjectFile[_MAX_PATH];
				sscanf (szLineBuff, "%s%s%f%f%f%f%f%f%f", szCommand, szObjectFile, &scale,
					&yaw, &pitch, &roll, &tx, &ty, &tz);

				// setup the 3D model
				nObjectCount++;
				CD3DModel *pModel = new CD3DModel;
#ifdef _DEBUG
				cout << "Loading model " << nObjectCount - 1 << " ..." << endl;
#endif
				if (FALSE == pModel->load3DModel (m_pD3DDevice, szObjectFile, 1.0))
				{
					return FALSE;
				}

				// build model world matrix
				D3DXMATRIX matWorld;
				D3DXVECTOR3 matRotationCenter (0.0f, 0.0f, 0.0f);
				D3DXQUATERNION matRotation;
				D3DXQuaternionRotationYawPitchRoll(&matRotation, D3DXToRadian(yaw), D3DXToRadian(pitch), D3DXToRadian(roll));
				D3DXVECTOR3  matTrans (tx, ty, tz);
				D3DXMatrixAffineTransformation (&matWorld, scale, &matRotationCenter, &matRotation, &matTrans);
				pModel->setWorldTransform (matWorld);

				// add model to list
				m_apModel.push_back (pModel);
			}
			else
			{
				// unimplemented or unrecognized command
			}
		}

		// Restore the original current directory
		SetCurrentDirectory (szOldDir);

		return TRUE;
	}

	return FALSE;
}

BOOL CD3DRenderer::displayD3D(float fTimeDelta)
{
	static DWORD dwFrameCnt = 0;
	static float fTimeElapsed = 0;
	static char szFPSString[25] = {0,};

	if( m_pD3DDevice ) // Only use Device methods if we have a valid device.
	{
		// Walking on the terrain: Adjust camera’s height so we
		// are standing 2m above the cell point we are
		// standing on.
		if (m_cCamera.getCameraType () == LANDOBJECT)
		{
			D3DXVECTOR3 pos;
			m_cCamera.getPosition (pos);
			pos.y = m_cTerrain.getHeight (pos.x, pos.z) + m_cCamera.GetCameraHeight();
			m_cCamera.setPosition (pos);
		}

		for (int iViewPort = 0; iViewPort < 2; iViewPort++)
		{
			// calculate camera for viewport
			CD3DCamera cam (m_cCamera);
			cam.strafe ((0 == iViewPort)?(-0.5f * m_rEyeOffset):(0.5f * m_rEyeOffset));

			// Set the projection matrix.
			m_persProjection._31 = ((0 == iViewPort)?1.0f:-1.0f) * m_rHorizOverlap [iViewPort];	// horizontal overlap
			m_persProjection._32 = ((0 == iViewPort)?1.0f:-1.0f) * m_rVertiAlign [iViewPort];	// vertical align
			m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &m_persProjection);

			// Update the view matrix representing the cameras 
			// new position/orientation.
			D3DXMATRIX view;
			cam.getViewMatrix(view);
			m_pD3DDevice->SetTransform(D3DTS_VIEW, &view);

			// set viewport
			m_pD3DDevice->SetViewport (&m_asViewPort[iViewPort]);

			// instruct the device to set each pixel on the back buffer black -
			// D3DCLEAR_TARGET: 0x00000000 (black) - and to set each pixel on
			// the depth buffer to a value of 1.0 - D3DCLEAR_ZBUFFER: 1.0f.
			m_pD3DDevice->Clear (0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, SKY_BLUE, 1.0f, 0);

			// begin scene rendering
			m_pD3DDevice->BeginScene (); 

			// perform terrain rendering
			m_cTerrain.renderTerrain ();

			// render all models
			for (unsigned int index = 0; index < m_apModel.size(); index++)
			{
				// preform model rendering
				m_apModel[index]->renderModel ();
			}

			// Render fps string
			RECT rect = {0, 0, 100, 25};
			m_pMsgStringFont->DrawText(NULL,
				szFPSString,      
				-1,				  // size of string or -1 indicates null terminating string
				&rect,            // rectangle text is to be formatted to in windows coords
				DT_TOP | DT_LEFT, // draw in the top left corner of the viewport
				0xffffffff);      // black text

			// end scene rendering
			m_pD3DDevice->EndScene ();

			// apply distortion shaders
			for (int iShader = 0; iShader <= 1; iShader++)
			{
				// pre-warp backbuffer
				drawWarpedBuffer (iShader, iViewPort);
			}
		}

		// Update: Compute the frames per second.
		dwFrameCnt++;
		fTimeElapsed += fTimeDelta;
		if(fTimeElapsed >= 1.0f)
		{
			sprintf(szFPSString, "%5.1f fps", (float)dwFrameCnt / fTimeElapsed);
			dwFrameCnt = 0;
			fTimeElapsed = 0.0f;
		}

		// swap the back and front buffers.
		m_pD3DDevice->Present (0, 0, 0, 0);

		return TRUE;
	}

	return FALSE;
}

void CD3DRenderer::drawWarpedBuffer (int iShader, int iViewPort)
{
	//D3DXSaveSurfaceToFile ("BackbufferImage.BMP", D3DXIFF_BMP, m_pBackBuffer, NULL, NULL);
	// capture viewport backbuffer as texture
	IDirect3DSurface9* pRenderTarget;
	m_pPreWarpBuffer->GetSurfaceLevel (0, &m_pBackBuffer);
	m_pD3DDevice->GetRenderTarget (0, &pRenderTarget);
	RECT viewPortRect;
	viewPortRect.left = m_asViewPort[iViewPort].X;
	viewPortRect.top = m_asViewPort[iViewPort].Y;
	viewPortRect.right = m_asViewPort[iViewPort].X + m_asViewPort[iViewPort].Width;
	viewPortRect.bottom = m_asViewPort[iViewPort].Y + m_asViewPort[iViewPort].Height;
	m_pD3DDevice->StretchRect (pRenderTarget, &viewPortRect, m_pBackBuffer, NULL, D3DTEXF_NONE);
	pRenderTarget->Release();

	// set viewport and pixel mappings
	m_pD3DDevice->SetViewport (&m_asViewPort[iViewPort]);

	float width = (float)m_asViewPort[iViewPort].Width;
	float height = (float)m_asViewPort[iViewPort].Height;
	float x = (float)m_asViewPort[iViewPort].X;
	float y = (float)m_asViewPort[iViewPort].Y;

	x -= 0.5f; y -= 0.5f; //correct for tex sampling
	VERT quad[4] =
	{
		{ x,		 y,			 0.5f, 1.0f, 0.0f, 0.0f},
		{ x + width, y,			 0.5f, 1.0f, 1.0f, 0.0f},
		{ x,		 y + height, 0.5f, 1.0f, 0.0f, 1.0f},
		{ x + width, y + height, 0.5f, 1.0f, 1.0f, 1.0f}
	};
	m_pD3DDevice->SetVertexDeclaration (m_pVertDeclaration);

	// apply distortion effects to viewport via pixel shaders
	m_pDistEffect->SetTexture ("preWarpBuffer", m_pPreWarpBuffer);

	if (0 == iShader) // angle correction
	{
		m_pDistEffect->SetTechnique (m_pDistEffect->GetTechniqueByName("RenderFrustumWarp"));
		m_pDistEffect->SetBool ("left", (0 == iViewPort));
		m_pDistEffect->SetFloat ("frustumOffset", m_rFrustumOffset);
	}
	else if (1 == iShader) // barrel distortion
	{
		m_pDistEffect->SetTechnique (m_pDistEffect->GetTechniqueByName("RenderBarrelWarp"));
		m_pDistEffect->SetFloat ("barrelFactor", m_rBarrelFactor);
	}
	else
	{
		return;	// unsupported shader
	}

	UINT cPasses;
	m_pD3DDevice->BeginScene();
	m_pDistEffect->Begin (&cPasses, 0);
	for(UINT p = 0; p < cPasses; p++)
	{
		m_pDistEffect->BeginPass (p);
		m_pD3DDevice->DrawPrimitiveUP (D3DPT_TRIANGLESTRIP, 2, quad, sizeof(VERT));
		m_pDistEffect->EndPass();
	}
	m_pDistEffect->End();
	m_pD3DDevice->EndScene();
}

void CD3DRenderer::cleanupD3D ()
{
	m_cTerrain.unloadTerrain ();
	for (unsigned int i = 0; i < m_apModel.size(); i++)
	{
		if (m_apModel[i])
			delete m_apModel[i];
	}
	m_apModel.clear ();
}

void CD3DRenderer::deInitD3D ()
{
	_SAFE_RELEASE ( m_pD3DDevice ); // Only use Device methods if we have a valid device.
	_SAFE_RELEASE ( m_pD3D );		// done with d3d9 object
	_SAFE_RELEASE ( m_pMsgStringFont );
	_SAFE_RELEASE ( m_pPreWarpBuffer );
	_SAFE_RELEASE ( m_pBackBuffer );
	_SAFE_RELEASE ( m_pDistEffect );
	_SAFE_RELEASE ( m_pVertDeclaration );
}

void CD3DRenderer::OnKeyDown (WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case (VK_HOME):
		ResetParameters ();
		break;
	case (VK_F1):
		m_nSelectedViewport = 0;
		break;
	case (VK_F2):
		m_nSelectedViewport = -1;
		break;
	case (VK_F3):
		m_nSelectedViewport = 1;
		break;
	case ('C'):
		m_rEyeOffset += 0.001f;
		if (m_rEyeOffset > 0.07f)
			m_rEyeOffset = 0.07f;
		break;
	case ('V'):
		m_rEyeOffset -= 0.001f;
		if (m_rEyeOffset < 0.05f)
			m_rEyeOffset = 0.05f;
		break;
	case ('Y'):
		m_rFrustumOffset += 0.01f;
		if (m_rFrustumOffset > 1.0f)
			m_rFrustumOffset = 1.0f;
		break;
	case ('H'):
		m_rFrustumOffset -= 0.01f;
		if (m_rFrustumOffset < 0.0f)
			m_rFrustumOffset = 0.0f;
		break;
	case ('Z'):
		if (m_nSelectedViewport <= 0)
		{
			m_rHorizOverlap[0] += 0.01f;
			if (m_rHorizOverlap[0] > 1.0f)
				m_rHorizOverlap[0] = 1.0f;
		}
		if (m_nSelectedViewport >= 0)
		{
			m_rHorizOverlap[1] += 0.01f;
			if (m_rHorizOverlap[1] > 1.0f)
				m_rHorizOverlap[1] = 1.0f;
		}
		break;
	case ('X'):
		if (m_nSelectedViewport <= 0)
		{
			m_rHorizOverlap[0] -= 0.01f;
			if (m_rHorizOverlap[0] < 0.0f)
				m_rHorizOverlap[0] = 0.0f;
		}
		if (m_nSelectedViewport >= 0)
		{
			m_rHorizOverlap[1] -= 0.01f;
			if (m_rHorizOverlap[1] < 0.0f)
				m_rHorizOverlap[1] = 0.0f;
		}
		break;
	case ('T'):
		if (m_nSelectedViewport <= 0)
		{
			m_rVertiAlign[0] += 0.01f;
			if (m_rVertiAlign[0] > 0.5f)
				m_rVertiAlign[0] = 0.5f;
		}
		if (m_nSelectedViewport >= 0)
		{
			m_rVertiAlign[1] += 0.01f;
			if (m_rVertiAlign[1] > 0.5f)
				m_rVertiAlign[1] = 0.5f;
		}
		break;
	case ('G'):
		if (m_nSelectedViewport <= 0)
		{
			m_rVertiAlign[0] -= 0.01f;
			if (m_rVertiAlign[0] < -0.5f)
				m_rVertiAlign[0] = -0.5f;
		}
		if (m_nSelectedViewport >= 0)
		{
			m_rVertiAlign[1] -= 0.01f;
			if (m_rVertiAlign[1] < -0.5f)
				m_rVertiAlign[1] = -0.5f;
		}
		break;
	case ('U'):
		m_rBarrelFactor *= 0.99f;
		if (m_rBarrelFactor < 0.0f)
		{
			if (m_rBarrelFactor > -2.0f)
				m_rBarrelFactor = -2.0f;
		}
		else
		{
			if (m_rBarrelFactor < 2.0f)
				m_rBarrelFactor = 2.0f;
		}
		break;
	case ('J'):
		m_rBarrelFactor *= 1.01f;
		break;
	default:
		break;
	};
}

void CD3DRenderer::AsyncKeyDown (float fTimeDelta)
{
	float rWalkingSensitivity = 3.0f;
	float rLookingSensitivity = 0.2f;

	// Control D3DCamera using keyboard
	if (::GetAsyncKeyState('W') & 0x8000f)
	{
		m_cCamera.walk (rWalkingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('S') & 0x8000f)
	{
		m_cCamera.walk (-rWalkingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('A') & 0x8000f)
	{
		m_cCamera.strafe (-rWalkingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('D') & 0x8000f)
	{
		m_cCamera.strafe (rWalkingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('R') & 0x8000f)
	{
		m_cCamera.fly (rWalkingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('F') & 0x8000f)
	{
		m_cCamera.fly (-rWalkingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState(VK_UP) & 0x8000f)
	{
		m_cCamera.pitch (rLookingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState(VK_DOWN) & 0x8000f)
	{
		m_cCamera.pitch (-rLookingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState(VK_LEFT) & 0x8000f)
	{
		m_cCamera.yaw (-rLookingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState(VK_RIGHT) & 0x8000f)
	{
		m_cCamera.yaw (rLookingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('Q') & 0x8000f)
	{
		m_cCamera.roll (rLookingSensitivity * fTimeDelta);
	}

	if (::GetAsyncKeyState('E') & 0x8000f)
	{
		m_cCamera.roll (-rLookingSensitivity * fTimeDelta);
	}
}

void CD3DRenderer::MouseMove (float rMoveX, float rMoveY)
{
	float rMouseSensitivity = 0.005f;

	m_cCamera.walk (-rMouseSensitivity * rMoveY);
	m_cCamera.strafe (rMouseSensitivity * rMoveX);
}

void CD3DRenderer::MouseSpin (float rSpinX, float rSpinY)
{
	float rMouseSensitivity = 0.005f;

	m_cCamera.yaw (rMouseSensitivity * rSpinX);
	m_cCamera.pitch (rMouseSensitivity * rSpinY);
}
