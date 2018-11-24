
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
		.nMaxFile = 1024,
		.lpstrFilter = "All\0*.*\0Image\0*.png\0Canvas\0*.ncvs\0",
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
		.nMaxFile = 1024,
		.lpstrFilter = "All\0*.*\0Image\0*.png\0Canvas\0*.ncvs\0",
		.nFilterIndex = 1,
		.nMaxFileTitle = 0,
		.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
	};
	return GetSaveFileName(&saveFileNameData);
}

void GUI(void)
{
	static mist_NESPixel pixel =
	{
		.paletteIndex = 0,
		.blockColor = 0
	};

	static mist_NESCanvas canvas =
	{
		.isForeground = true,

		.pixelCanvas = { { 0 } },
		.paletteBlockIndices = { { 0 } },

		.palette = { {0, 1, 2, 3,  0, 4, 5, 6,  0, 7, 8, 9,  0, 10, 11, 12},{ 0, 1, 2, 3,  0, 4, 5, 6,  0, 7, 8, 9,  0, 10, 11, 12 } },
		.width = 128,
		.height = 128
	};

	mist_Vec2 canvasSize = { .x = 768.0f,.y = 768.0f };
	GUI_NESColorPicker(&canvas, &pixel, (mist_Vec2) { .x = 0.0f, .y = g_ScreenDimensions.y * 0.5f - 256.0f }, (mist_Vec2) { .x = 100.0f, .y = 512.0f });
	GUI_NESPalette(&canvas, &pixel, (mist_Vec2) { .x = g_ScreenDimensions.x - 100.0f, .y = g_ScreenDimensions.y * 0.5f - 256.0f }, (mist_Vec2) { .x = 100.0f, .y = 512.0f });
	GUI_NESCanvas(&canvas, &pixel, (mist_Vec2) { .x = g_ScreenDimensions.x * 0.5f - canvasSize.x*0.5f, .y = g_ScreenDimensions.y * 0.5f - canvasSize.y * 0.5f }, canvasSize);


	static const char* toolBars[] =
	{
		"Open",
		"Save",
		"Export",
		"Foreground"
	};

	enum mist_Toolbar
	{
		Toolbar_Open,
		Toolbar_Save,
		Toolbar_Export,
		Toolbar_IsBackground
	};
	
	#define GUIConfig_ToolbarWidth 110.0f
	#define GUIConfig_ToolbarHeight 25.0f
	
	int8_t selectedTab = GUI_Toolbar((mist_Vec2) { 0.0f, g_ScreenDimensions.y - GUIConfig_ToolbarHeight },
							(mist_Vec2) { GUIConfig_ToolbarWidth, GUIConfig_ToolbarHeight }, toolBars, ARRAYSIZE(toolBars));
	if (selectedTab == Toolbar_Open)
	{
		char path[1024] = { 0 };
		if (OpenDialog(path))
		{
			FILE* file = fopen(path, "rb");
			fread(&canvas, sizeof(mist_NESCanvas), 1, file);
			fclose(file);

			canvas.isForeground = !canvas.isForeground;
			if (canvas.isForeground)
			{
				toolBars[Toolbar_IsBackground] = "Foreground";
			}
			else
			{
				toolBars[Toolbar_IsBackground] = "Background";
			}
		}
	}
	else if (selectedTab == Toolbar_Save)
	{
		char path[1024] = { 0 };
		if (SaveDialog(path))
		{
			FILE* filePtr = fopen(path, "wb");
			fwrite(&canvas, sizeof(mist_NESCanvas), 1, filePtr);
			fclose(filePtr);
		}
	}
	else if (selectedTab == Toolbar_Export)
	{
		char path[1024] = { 0 };
		if (SaveDialog(path))
		{
			// Palette:
			unsigned int writeHead = 0;
			#define PALETTE_STRING_SIZE 1024 * 10
			char paletteString[PALETTE_STRING_SIZE];

			const char* header = "PALETTE_DATA:";
			memcpy(paletteString, header, strlen(header));
			writeHead += strlen(header);

			for (unsigned int i = 0; i < 2; i++)
			{
				const char* db = "\n\t.db ";
				memcpy(paletteString + writeHead, db, strlen(db));
				writeHead += strlen(db);

				for (unsigned int j = 0; j < 15; j++)
				{
					writeHead += snprintf(paletteString + writeHead, PALETTE_STRING_SIZE - writeHead, "$%02.2x,", canvas.palette[i][j]);
				}
				writeHead += snprintf(paletteString + writeHead, PALETTE_STRING_SIZE - writeHead, "$%02.2x", canvas.palette[i][15]);
			}

			const char* paletteExtension = ".palette\0";
			unsigned int extensionPos = strlen(path);
			memcpy(path + extensionPos, paletteExtension, strlen(paletteExtension) + 1);

			FILE* filePtr = fopen(path, "w");
			fwrite(paletteString, strlen(paletteString), 1, filePtr);
			fclose(filePtr);

#define PAGE_SIZE 256 * 2 * 8
			uint8_t chrData[2 * PAGE_SIZE];
			for (unsigned int background = 0; background < 2; background++)
			{
				for (unsigned int blockY = 0; blockY < 16; blockY++)
				{
					for (unsigned int blockX = 0; blockX < 16; blockX++)
					{
						unsigned int start = ((15 - blockY) * 16 * 64 + blockX * 8);

						for (unsigned int bitPlane = 0; bitPlane < 2; bitPlane++)
						{
							for (unsigned int y = 0; y < 8; y++)
							{
								uint8_t row = 0;
								for (unsigned int x = 0; x < 8; x++)
								{
									unsigned int pixel = canvas.pixelCanvas[background][start + (7 - y) * 16 * 8 + x];
									row |= ((pixel & (1 << bitPlane)) >> bitPlane) << (7 - x);
								}

								uint16_t index = background << 12 | blockY << 8 | blockX << 4 | bitPlane << 3 | y;

								chrData[index] = row;
							}
						}
					}
				}
			}

			const char* patternExtension = ".chr\0";
			memcpy(path + extensionPos, patternExtension, strlen(patternExtension) + 1);

			filePtr = fopen(path, "wb");
			fwrite(chrData, ARRAYSIZE(chrData), 1, filePtr);
			fclose(filePtr);
		}
	}
	else if (selectedTab == Toolbar_IsBackground)
	{
		canvas.isForeground = !canvas.isForeground;
		if (canvas.isForeground)
		{
			toolBars[Toolbar_IsBackground] = "Foreground";
		}
		else
		{
			toolBars[Toolbar_IsBackground] = "Background";
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

#define SHOW_CONSOLE 0

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
#else
	(void)prevInstance;
	(void)args;
	(void)moreArgs;
#endif

	// Config
	#define winConfig_IsFullscreen false
	#define winConfig_WindowName "Mist Vulkan"

	#define winConfig_Width 1024
	#define winConfig_Height 900

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