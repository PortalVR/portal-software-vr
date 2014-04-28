
// StereoVisualizer.h : main header file for the StereoVisualizer application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CStereoVisualizerApp:
// See StereoVisualizer.cpp for the implementation of this class
//

class CStereoVisualizerApp : public CWinApp
{
public:
	CStereoVisualizerApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

//// Application state machine
//typedef enum _tag_EStateMachine {
//	_APP_START = -1,		// dummy state
//	_D3D_,
//	_MODEL_LOADED
//}EStateMachine;

extern CStereoVisualizerApp		theApp;
//extern EStateMachine			g_eAppState;
