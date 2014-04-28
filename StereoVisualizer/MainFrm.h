
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "D3DView.h"

class CMainFrame : public CFrameWnd
{
protected: 
	DECLARE_DYNAMIC(CMainFrame)

public:
	CMainFrame();
	virtual ~CMainFrame();

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Implementation
public:
	bool	m_fD3DInitialized;
	bool	m_fFullScreen;
	bool	m_fModelLoaded;
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar		m_wndStatusBar;
	CD3DView		m_wndView;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);

public:
	afx_msg void OnFileOpenModel();
	afx_msg void OnUpdateFileOpenModel(CCmdUI *pCmdUI);
	afx_msg void OnFileCloseModel();
	afx_msg void OnUpdateFileCloseModel(CCmdUI *pCmdUI);
	afx_msg void OnViewFullscreen();
	afx_msg void OnUpdateViewFullscreen(CCmdUI *pCmdUI);
};
