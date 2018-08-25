
// Platform
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <WinUser.h>
#include <Commdlg.h>

// Standard lib
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

// Vulkan
#include "VkAllocator.h"
#include "VkRenderer.h"

// GUI
#include "MistGUI.h"


HWND g_VulkanWindow;
mist_Vec2 g_ScreenDimensions;
bool g_IsMinimized;

#define WIN_CHECK(a) \
	BOOL result = a; \
	if(result == FALSE) \
	{ \
		assert(false); \
	}

void mist_Print(const char* message)
{
	printf("%s\n", message);
}

bool OpenDialog(char* path)
{
	OPENFILENAME openFileNameData =
	{
		.lStructSize = sizeof(OPENFILENAME),
		.lpstrFile = path,
		.nMaxFile = ARRAYSIZE(path),
		.lpstrFilter = "All\0*.*\0Image\0*.png\0",
		.nFilterIndex = 1,
		.nMaxFileTitle = 0,
		.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
	};
	return GetOpenFileName(&openFileNameData);
}

bool SaveDialog(char* path)
{
	OPENFILENAME saveFileNameData =
	{
		.lStructSize = sizeof(OPENFILENAME),
		.lpstrFile = path,
		.nMaxFile = ARRAYSIZE(path),
		.lpstrFilter = "All\0*.*\0Image\0*.png\0",
		.nFilterIndex = 1,
		.nMaxFileTitle = 0,
		.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
	};
	return GetSaveFileName(&saveFileNameData);
}

void GUI(void)
{
	const char* toolBars[] =
	{
		"Open",
		"Save",
		"Export"
	};

	enum mist_Toolbar
	{
		Toolbar_Open,
		Toolbar_Save,
		Toolbar_Export
	};
	
	#define GUIConfig_ToolbarWidth 80.0f
	#define GUIConfig_ToolbarHeight 24.0f
	
	int8_t selectedTab = GUI_Toolbar((mist_Vec2) { GUIConfig_ToolbarWidth * 0.5f, g_ScreenDimensions.y - GUIConfig_ToolbarHeight * 0.5f },
							(mist_Vec2) { GUIConfig_ToolbarWidth, GUIConfig_ToolbarHeight }, toolBars, ARRAYSIZE(toolBars));
	if (selectedTab == Toolbar_Open)
	{
		char path[256] = { 0 };
		if (OpenDialog(path))
		{
			// TODO:
		}
	}
	else if (selectedTab == Toolbar_Save)
	{
		char path[256] = { 0 };
		if (SaveDialog(path))
		{
			// TODO:
		}
	}
	else if (selectedTab == Toolbar_Export)
	{
		char path[256] = { 0 };
		if (SaveDialog(path))
		{
			// TODO:
		}
	}
}

HWND mist_CreateWindow(const char* windowName, bool showFullscreen, int width, int height, HINSTANCE hinstance, WNDPROC wndproc)
{
	static const char* WindowClassName = "VkWindow";
	bool isFullscreen = showFullscreen;

	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = WindowClassName;
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		printf("Could not register window class!\n");
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (isFullscreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = screenWidth;
		dmScreenSettings.dmPelsHeight = screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if ((width != screenWidth) && (height != screenHeight))
		{
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					isFullscreen = false;
				}
				else
				{
					return NULL;
				}
			}
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (isFullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect;
	windowRect.left = 0L;
	windowRect.top = 0L;
	windowRect.right = isFullscreen ? (long)screenWidth : (long)width;
	windowRect.bottom = isFullscreen ? (long)screenHeight : (long)height;

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	HWND window = CreateWindowEx(0,
		WindowClassName,
		windowName,
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!isFullscreen)
	{
		// Center on screen
		int x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		int y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	if (!window)
	{
		printf("Could not create window!\n");
		return NULL;
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
		g_MouseState = MouseState_Down;
		break;

	case WM_LBUTTONUP:
		g_MouseState = MouseState_Up;
		break;

	case WM_SIZE:
		g_ScreenDimensions =
		(mist_Vec2)
		{
			.x = (float)LOWORD(lParam),
			.y = (float)HIWORD(lParam)
		};

		g_IsMinimized = wParam == SIZE_MINIMIZED;
		break;

	case WM_MOUSEMOVE:
	{
		int xPoint = GET_X_LPARAM(lParam);
		int yPoint = GET_Y_LPARAM(lParam);

		g_MousePosition =
		(mist_Vec2)
		{
			.x = (float)(xPoint),
			.y = g_ScreenDimensions.y - (float)(yPoint)
		};

		break;
	}
	}

	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void Input_Refresh(void)
{
	g_MouseState = g_MouseState == MouseState_Down ? MouseState_Held : g_MouseState;
	g_MouseState = g_MouseState == MouseState_Up ? MouseState_None : g_MouseState;
}

#define SHOW_CONSOLE 1

#if SHOW_CONSOLE
#define MIST_WIN_PROC(void) \
	int main(void)
#else
#define MIST_WIN_PROC(void) \
	int CALLBACK WinMain(\
	_In_ HINSTANCE mist_WinInstance, \
	_In_ HINSTANCE prevInstance, \
	_In_ LPSTR     args, \
	_In_ int       moreArgs)
#endif

MIST_WIN_PROC(void)
{
#if SHOW_CONSOLE
	HINSTANCE mist_WinInstance = GetModuleHandle(NULL);
#endif

	// Config
	#define winConfig_IsFullscreen false
	#define winConfig_WindowName "Mist Vulkan"

	#define winConfig_Width 900
	#define winConfig_Height 700

	g_VulkanWindow = mist_CreateWindow(winConfig_WindowName, winConfig_IsFullscreen, winConfig_Width, winConfig_Height, mist_WinInstance, WndProc);
	VkRenderer_Init(mist_WinInstance, g_VulkanWindow, winConfig_Width, winConfig_Height);

	g_ScreenDimensions = (mist_Vec2) { .x = (float)winConfig_Width, .y = (float)winConfig_Height };

	mist_Print("Creating descriptor pool...");

	// Game stuff woo!
	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived)
	{
		if (!g_IsMinimized)
		{
			VkRenderer_ClearInstances();
			GUI();
			VkRenderer_Draw((uint32_t)g_ScreenDimensions.x, (uint32_t)g_ScreenDimensions.y);
		}

		Input_Refresh();
		// Handle windows messages...
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				quitMessageReceived = true;
				break;
			}
		}
	}

	mist_Print("Shutting down...");

	VkRenderer_Kill();

	mist_Print("Shutdown!");

	return 0;
} 