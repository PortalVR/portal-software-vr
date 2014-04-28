#pragma once

#include "D3Dutility.h"
#include "D3DCamera.h"
#include "D3DModel.h"

// CD3DView
class CD3DView : public CWnd
{
// Construction
public:
	CD3DView();
	virtual ~CD3DView();

	// Attributes
public:

	// Operations
public:

	// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Implementation
protected:
	IDirect3DDevice9*	m_pD3DDevice;
	BOOL				m_fWindowed;
	int					m_nWidth;
	int					m_nHeight;

	CD3DCamera			m_3dCamera;
	CD3DModel			m_3dModel;
public:
	// exposed routines
	BOOL InitD3D(int nWidth = 0, int nHeight = 0, BOOL fWindowed = TRUE);
	BOOL SetupD3D (const char szModelPath[], const char szModelFile []);
	BOOL DisplayD3D();
	void CleanupD3D ();
	void DeInitD3D ();

	// Generated message map functions
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint ();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};
