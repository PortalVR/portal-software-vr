// D3DView.cpp : implementation file
//

#include "stdafx.h"
#include "StereoVisualizer.h"
#include "D3DView.h"

// CD3DView

BEGIN_MESSAGE_MAP(CD3DView, CWnd)
	ON_WM_PAINT ()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

CD3DView::CD3DView()
{
	m_fWindowed = TRUE;
	m_pD3DDevice = NULL;
}

CD3DView::~CD3DView()
{
	CleanupD3D();
	DeInitD3D();
}

BOOL CD3DView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	if( !CWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

BOOL CD3DView::InitD3D(int nWidth, int nHeight, BOOL fWindowed)
{
	CRect rectClient;
	GetClientRect (&rectClient);

	if (0 == nWidth)
	{
		m_nWidth = rectClient.Width ();
	}
	else
	{
		m_nWidth = nWidth;
	}

	if (0 == nHeight)
	{
		m_nHeight = rectClient.Height ();
	}
	else
	{
		m_nHeight = nHeight;
	}

	m_fWindowed = fWindowed;

	// Step 1: Create the IDirect3D9 object.
	IDirect3D9* d3d9 = 0;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	if( !d3d9 )
	{
		return FALSE;
	}

	// Step 2: Check for hardware vp.
	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	int vp = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Step 3: Fill out the D3DPRESENT_PARAMETERS structure.
	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth            = m_nWidth;
	d3dpp.BackBufferHeight           = m_nHeight;
	d3dpp.BackBufferFormat           = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount            = 1;
	d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality         = 0;
	d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD; 
	d3dpp.hDeviceWindow              = this->m_hWnd;
	d3dpp.Windowed                   = m_fWindowed;
	d3dpp.EnableAutoDepthStencil     = TRUE; 
	d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
	d3dpp.Flags                      = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Step 4: Create the device.
	HRESULT hr = d3d9->CreateDevice(
		D3DADAPTER_DEFAULT, // primary adapter
		D3DDEVTYPE_HAL,         // device type
		this->m_hWnd,               // window associated with device
		vp,                 // vertex processing
		&d3dpp,             // present parameters
		&m_pD3DDevice);            // return created device

	if( FAILED(hr) )
	{
		d3d9->Release(); // done with d3d9 object
		m_pD3DDevice = NULL;
		return FALSE;
	}

	d3d9->Release(); // done with d3d9 object

	return TRUE;
}

BOOL CD3DView::SetupD3D (const char szModelPath[], const char szModelFile [])
{
	if( m_pD3DDevice ) // Only use Device methods if we have a valid device.
	{
		// Setup the building model
		if (FALSE == m_3dModel.Load3DModel (m_pD3DDevice, szModelPath, szModelFile))
		{
			return FALSE;
		}

		// Set the projection matrix.
		D3DXMATRIX proj;
		D3DXMatrixPerspectiveFovLH(
			&proj,
			D3DX_PI * 0.25f, // 45 - degree
			(float)m_nWidth / (float)m_nHeight,
			1.0f,
			1000.0f);
		m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &proj);

		// setup light sources
		// ambient light
		m_pD3DDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(128, 128, 128));
		// directional light
		D3DXVECTOR3 dir(0.0f, 0.0f, 1.0f);
		D3DLIGHT9 light = InitDirectionalLight(dir, GREY);
		m_pD3DDevice->SetLight(0, &light);
		m_pD3DDevice->LightEnable(0, TRUE);

		// set rendering parameters
		m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);	// cull clockise triangles
		//m_pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME); // render in wireframe mode
		m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE); 		// switch on light
		m_pD3DDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);	// enable specular reflection
		m_pD3DDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE); // enable scale independent lighting

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CD3DView::DisplayD3D()
{
	if( m_pD3DDevice ) // Only use Device methods if we have a valid device.
	{
		// Update the view matrix representing the cameras 
		// new position/orientation.
		D3DXMATRIX V;
		m_3dCamera.getViewMatrix(&V);
		m_pD3DDevice->SetTransform(D3DTS_VIEW, &V);

		// Instruct the device to set each pixel on the back buffer black -
		// D3DCLEAR_TARGET: 0x00000000 (black) - and to set each pixel on
		// the depth buffer to a value of 1.0 - D3DCLEAR_ZBUFFER: 1.0f.
		m_pD3DDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, WHITE, 1.0f, 0);
		// begin scene rendering
		m_pD3DDevice->BeginScene (); 

		// preform model rendering
		m_3dModel.RenderModel (this->m_hWnd);

		// end scene rendering
		m_pD3DDevice->EndScene ();
		// Swap the back and front buffers.
		m_pD3DDevice->Present(0, 0, 0, 0);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void CD3DView::CleanupD3D ()
{
	m_3dModel.UnLoad3DModel ();
}

void CD3DView::DeInitD3D ()
{
	if( m_pD3DDevice ) // Only use Device methods if we have a valid device.
	{
		m_pD3DDevice->Release ();
		m_pD3DDevice = NULL;
	}
}

// CD3DView message handlers
void CD3DView::OnPaint()
{
	CPaintDC dc(this);  // device context for painting
	// refresh D3D scene rendering
	DisplayD3D ();
}

void CD3DView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	float fTimeDelta = 0.10f;

	// Control D3DCamera using keyboard
	if( ::GetAsyncKeyState('W') & 0x8000f )
		m_3dCamera.walk(40.0f * fTimeDelta);

	if( ::GetAsyncKeyState('S') & 0x8000f )
		m_3dCamera.walk(-10.0f * fTimeDelta);

	if( ::GetAsyncKeyState('A') & 0x8000f )
		m_3dCamera.strafe(-10.0f * fTimeDelta);

	if( ::GetAsyncKeyState('D') & 0x8000f )
		m_3dCamera.strafe(10.0f * fTimeDelta);

	if( ::GetAsyncKeyState('R') & 0x8000f )
		m_3dCamera.fly(10.0f * fTimeDelta);

	if( ::GetAsyncKeyState('F') & 0x8000f )
		m_3dCamera.fly(-10.0f * fTimeDelta);

	if( ::GetAsyncKeyState(VK_UP) & 0x8000f )
		m_3dCamera.pitch(0.1f * fTimeDelta);

	if( ::GetAsyncKeyState(VK_DOWN) & 0x8000f )
		m_3dCamera.pitch(-0.1f * fTimeDelta);

	if( ::GetAsyncKeyState(VK_LEFT) & 0x8000f )
		m_3dCamera.yaw(-0.1f * fTimeDelta);

	if( ::GetAsyncKeyState(VK_RIGHT) & 0x8000f )
		m_3dCamera.yaw(0.1f * fTimeDelta);

	if( ::GetAsyncKeyState('Q') & 0x8000f )
		m_3dCamera.roll(0.2f * fTimeDelta);

	if( ::GetAsyncKeyState('E') & 0x8000f )
		m_3dCamera.roll(-0.2f * fTimeDelta);

	// refresh D3D scene rendering
	DisplayD3D ();

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}
