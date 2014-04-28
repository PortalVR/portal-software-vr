#define WIN32_LEAN_AND_MEAN

// Include the Windows header file; this has all the Win32 API
// structures, types, and function declarations we need to program
// Windows.
#include <windows.h>
#include <MMSystem.h>
#include <io.h>
#include <fcntl.h> 
#include "Common.h"
#include "D3DSettings.h"
#include "D3DRenderer.h"

// The main window handle
// This is used to identify the created application window.
HWND g_hMainWnd = 0;

// instance of Direct3D engine
CD3DSettings g_cD3DSettings;
CD3DRenderer g_cD3DEngine (g_cD3DSettings);

// Wraps the code necessary to initialize a Windows
// application. Function returns true if initialization
// was successful; otherwise, it returns false.
bool InitWindowsApp(HINSTANCE instanceHandle, int show);

// The window procedure handles events our window receives.
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Windows equivalant to main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
#ifdef _DEBUG
	int hCrt;
	FILE *hf;

	AllocConsole();
	hCrt = _open_osfhandle ((long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	hf = _fdopen ( hCrt, "w" );
	*stdout = *hf;
	int i = setvbuf ( stdout, NULL, _IONBF, 0 );
#endif

	// First call our wrapper function (InitWindowsApp) to create
	// and initialize the main application window, passing in the
	// hInstance and nShowCmd values as arguments.
	nShowCmd = SW_SHOWMAXIMIZED;
	if(!InitWindowsApp(hInstance, nShowCmd))
		return -1;

	// setup the renderer
	char szAssetFileName[_MAX_PATH];
	sscanf (pCmdLine, "%s", szAssetFileName);
	if (FALSE == g_cD3DEngine.setupD3D (szAssetFileName))
		return -1;

	// Once our application has been created and initialized, we
	// enter the message loop. We stay in the message loop until
	// a WM_QUIT message is received, indicating the application
	// should be terminated.
	// Loop until we get a WM_QUIT message. The function
	// GetMessage will only return 0 (false) when a WM_QUIT message
	// is received, which effectively exits the loop. The function
	// returns -1 if there is an error. Also, note that GetMessage
	// puts the application thread to sleep until there is a
	// message.
	MSG msg;
	::ZeroMemory(&msg, sizeof(MSG));

	static float lastTime = (float)timeGetTime();

	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{	
			float currTime  = (float)timeGetTime();	//	system time in milli sec
			float timeDelta = (currTime - lastTime)*0.001f; // time in micro sec

			g_cD3DEngine.AsyncKeyDown (0.05f + timeDelta);
			g_cD3DEngine.displayD3D (timeDelta);

			lastTime = currTime;
		}
	}

	return (int)msg.wParam;
}

bool InitWindowsApp(HINSTANCE instanceHandle, int show)
{
	// The first task to creating a window is to describe some of its
	// characteristics by filling out a WNDCLASS structure.
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instanceHandle;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = "EngineAppClass";

	// Next, we register this WNDCLASS instance with Windows so
	// that we can create a window based on it.
	if(!RegisterClass(&wc))
	{
		MessageBox(0, "RegisterClass FAILED", 0, 0);
		return false;
	}

	// With our WNDCLASS instance registered, we can create a
	// window with the CreateWindow function. This function
	// returns a handle to the window it creates (an HWND).
	// If the creation failed, the handle will have the value
	// of zero. A window handle is a way to refer to the window,
	// which is internally managed by Windows. Many of the Win32 API
	// functions that operate on windows require an HWND so that
	// they know what window to act on.
	g_hMainWnd = CreateWindow(
		"EngineAppClass",	// Registered WNDCLASS instance to use.
		"Stereo Visualizer",	// window title
		WS_OVERLAPPEDWINDOW,// style flags
		CW_USEDEFAULT,		// x-coordinate
		CW_USEDEFAULT,		// y-coordinate
		CW_USEDEFAULT,		// width
		CW_USEDEFAULT,		// height
		0,					// parent window
		0,					// menu handle
		instanceHandle,		// app instance
		0);					// extra creation parameters

	if(g_hMainWnd == 0)
	{
		MessageBox(0, "CreateWindow FAILED", 0, 0);
		return false;
	}

	// Even though we just created a window, it is not initially
	// shown. Therefore, the final step is to show and update the
	// window we just created, which can be done with the following
	// two function calls. Observe that we pass the handle to the
	// window we want to show and update so that these functions know
	// which window to show and update.
	ShowWindow(g_hMainWnd, show);
	UpdateWindow(g_hMainWnd);

	RECT clientRect;
	GetClientRect (g_hMainWnd, &clientRect);

	if (FALSE == g_cD3DEngine.initD3D (g_hMainWnd, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, TRUE))
		return false;

	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool fLeftMouseDown;
	static bool fRightMouseDown;
	static POINT ptPrevMousePos;
	static POINT ptCurrMousePos;
	// Handle some specific messages. Note that if we handle a
	// message, we should return 0.
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			// In the case the Escape key was pressed, then
			// destroy the main application window.
			DestroyWindow (g_hMainWnd);
			break;
		default:
			g_cD3DEngine.OnKeyDown (wParam, lParam);
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		fLeftMouseDown = true;
		ptPrevMousePos.x = ptCurrMousePos.x = LOWORD (lParam);
		ptPrevMousePos.y = ptCurrMousePos.y = HIWORD (lParam);
		break;
	case WM_LBUTTONUP:
		fLeftMouseDown = false;
		break;
	case WM_RBUTTONDOWN:
		fRightMouseDown = true;
		ptPrevMousePos.x = ptCurrMousePos.x = LOWORD (lParam);
		ptPrevMousePos.y = ptCurrMousePos.y = HIWORD (lParam);
		break;
	case WM_RBUTTONUP:
		fRightMouseDown = false;
		break;
	case WM_MOUSEMOVE:
			ptCurrMousePos.x = LOWORD (lParam);
			ptCurrMousePos.y = HIWORD (lParam);
			if (fLeftMouseDown)
			{
				g_cD3DEngine.MouseSpin (float(ptCurrMousePos.x - ptPrevMousePos.x),
					float(ptCurrMousePos.y - ptPrevMousePos.y));
			}
			if (fRightMouseDown)
			{
				g_cD3DEngine.MouseMove (float(ptCurrMousePos.x - ptPrevMousePos.x),
					float(ptCurrMousePos.y - ptPrevMousePos.y));
			}
			ptPrevMousePos.x = ptCurrMousePos.x;
            ptPrevMousePos.y = ptCurrMousePos.y;
			break;
	case WM_CLOSE:
			DestroyWindow (g_hMainWnd);
			break;
	case WM_DESTROY:
		// In the case of a destroy message, then send a
		// quit message, which will terminate the message loop.
		PostQuitMessage(0);
		break;
	default:
		// Forward any other messages we did not handle above to the
		// default window procedure. Note that our window procedure
		// must return the return value of DefWindowProc.
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}

	return 0;
}
