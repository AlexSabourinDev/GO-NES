
// Platform
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinUser.h>

// Standard lib
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

// Vulkan
#include "VkAllocator.h"
#include "VkRenderer.h"

void mist_Print(const char* message)
{
	printf("%s\n", message);
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
	}

	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

#define SHOW_CONSOLE 1

#if SHOW_CONSOLE
#define MIST_WIN_PROC() \
	int main()
#else
#define MIST_WIN_PROC() \
	int CALLBACK WinMain(\
	_In_ HINSTANCE mist_WinInstance, \
	_In_ HINSTANCE prevInstance, \
	_In_ LPSTR     args, \
	_In_ int       moreArgs)
#endif

MIST_WIN_PROC()
{
#if SHOW_CONSOLE
	HINSTANCE mist_WinInstance = GetModuleHandle(NULL);
#endif

	// Config
	#define		winConfig_IsFullscreen false
	#define		winConfig_Width 900
	#define		winConfig_Height 700
	#define		winConfig_WindowName "Mist Vulkan"

	HWND vulkanWindow = mist_CreateWindow(winConfig_WindowName, winConfig_IsFullscreen, winConfig_Width, winConfig_Height, mist_WinInstance, WndProc);
	VkRenderer_Init(mist_WinInstance, vulkanWindow, winConfig_Width, winConfig_Height);

	mist_Print("Creating descriptor pool...");

	// Game stuff woo!
	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived)
	{
		static float s = 0.0f;
		static float d = 1.0f;
		s += 0.001f * d;

		if (s >= 1.0f || s <= -1.0) d *= -1.0f;


		VkRenderer_ClearInstances();
		VkRenderer_AddInstance(VkMesh_Rect, (mist_Vec2) { 0.0f, 0.0f }, (mist_Vec2) { 0.1f, 0.1f });
		VkRenderer_AddInstance(VkMesh_Rect, (mist_Vec2) { s, 0.0f }, (mist_Vec2) { 0.1f, 0.1f });

		VkRenderer_Draw();

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