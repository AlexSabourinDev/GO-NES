
// Standard lib
#include <stdio.h>

// Platform
#include <Windows.h>

// Vulkan!
#define VK_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <Vulkan/Vulkan.h>

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
					return nullptr;
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
		return nullptr;
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
	HINSTANCE mist_WinInstance = GetModuleHandle(NULL); \
	int main()
#else
	int CALLBACK WinMain(\
	_In_ HINSTANCE mist_WinInstance, \
	_In_ HINSTANCE, \
	_In_ LPSTR, \
	_In_ int)

#endif


void mist_Print(const char* message)
{
	printf("%s\n", message);
}

VKAPI_ATTR VkBool32 VKAPI_CALL mist_VkDebugReportCallback(
	VkDebugReportFlagsEXT       ,
	VkDebugReportObjectTypeEXT  ,
	uint64_t                    ,
	size_t                      ,
	int32_t                     ,
	const char*                 pLayerPrefix,
	const char*                 pMessage,
	void*                       )
{
	mist_Print("Validation error!");
	mist_Print(pLayerPrefix);
	mist_Print(pMessage);
	return VK_FALSE;
}

#define NO_ALLOCATOR nullptr

template< typename Type >
struct mist_ScopedMemory
{
	mist_ScopedMemory(Type* mem)
		: data(mem)
	{
	}

	~mist_ScopedMemory()
	{
		free(data);
	}

	Type* operator->()
	{
		return data;
	}

	operator Type*()
	{
		return data;
	}

	Type* data;
};

MIST_WIN_PROC()
{
	// Config
	const bool  winConfig_IsFullscreen = false;
	const uint32_t   winConfig_Width        = 900;
	const uint32_t   winConfig_Height       = 700;
	const char* winConfig_WindowName   = "Mist Vulkan";

	const char* vkConfig_AppName    = "NesEditor";
	const char* vkConfig_EngineName = "Mist";

	const bool  vkConfig_EnableDebugExtensions  = true;
	const bool  vkConfig_EnableValidationLayers = true;

	const char* vkConfig_InstanceExtensions[]      = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const char* vkConfig_InstanceDebugExtensions[] = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	const uint32_t   vkConfig_MaxInstanceExtensions     = ARRAYSIZE(vkConfig_InstanceExtensions) + ARRAYSIZE(vkConfig_InstanceDebugExtensions);

	const char* vkConfig_DeviceExtensions[]  = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const uint32_t   vkConfig_MaxDeviceExtensions = ARRAYSIZE(vkConfig_DeviceExtensions);

	const char* vkConfig_ValidationLayers[]  = { "VK_LAYER_LUNARG_standard_validation" };
	const uint32_t   vkConfig_MaxValidationLayers = ARRAYSIZE(vkConfig_ValidationLayers);

	HWND vulkanWindow = mist_CreateWindow(winConfig_WindowName, winConfig_IsFullscreen, winConfig_Width, winConfig_Height, mist_WinInstance, WndProc);

	// Vulkan setup! Lets go!
	mist_Print("Vulkan Startup...");

	mist_Print("Creating VkInstance...");

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = vkConfig_AppName;
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = vkConfig_EngineName;
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	mist_Print("Creating instance layers...");
	const char* instanceExtensions[vkConfig_MaxInstanceExtensions] = {};
	int extensionCount = 0;

	for (uint32_t i = 0; i < ARRAYSIZE(vkConfig_InstanceExtensions); i++)
	{
		instanceExtensions[i] = vkConfig_InstanceExtensions[i];
	}
	extensionCount += ARRAYSIZE(vkConfig_InstanceExtensions);

	if (vkConfig_EnableDebugExtensions)
	{
		mist_Print("Adding Debug Extensions...");
		for (uint32_t i = 0; i < ARRAYSIZE(vkConfig_InstanceDebugExtensions); i++)
		{
			mist_Print(vkConfig_InstanceDebugExtensions[i]);

			const uint32_t offset = ARRAYSIZE(vkConfig_InstanceExtensions);
			instanceExtensions[offset + i] = vkConfig_InstanceDebugExtensions[i];
		}
		extensionCount += ARRAYSIZE(vkConfig_InstanceDebugExtensions);

		mist_Print("Adding Debug Extensions Done!");
	}

	const char* validationLayers[vkConfig_MaxValidationLayers];
	int validationLayerCount = 0;

	if (vkConfig_EnableValidationLayers)
	{
		mist_Print("Adding validation layers...");
		for (uint32_t i = 0; i < ARRAYSIZE(vkConfig_ValidationLayers); i++)
		{
			mist_Print(vkConfig_ValidationLayers[i]);

			validationLayers[i] = vkConfig_ValidationLayers[i];
		}
		validationLayerCount += ARRAYSIZE(vkConfig_ValidationLayers);

		mist_Print("Adding validation layers done!");
	}

	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = instanceExtensions;
	createInfo.enabledLayerCount = validationLayerCount;
	createInfo.ppEnabledLayerNames = validationLayers;
	
	VkInstance vkInstance;
	VkResult instanceCreated = vkCreateInstance(&createInfo, NO_ALLOCATOR, &vkInstance);
	if (VK_SUCCESS != instanceCreated)
	{
		mist_Print("Error: Failed to create vkInstance!");
		return -1;
	}

	mist_Print("Created vkInstance!");

	// TODO: Check why vkCreateDebugReportCallbackEXT isn't available
	/*VkDebugReportCallbackEXT debugReportCallback;
	if (vkConfig_EnableValidationLayers)
	{
		mist_Print("Creating debug report callback...");

		VkDebugReportCallbackCreateInfoEXT debugReportInfo = {};
		debugReportInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		debugReportInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugReportInfo.pfnCallback = mist_VkDebugReportCallback;

		VkResult createDebugReportSuccess = vkCreateDebugReportCallbackEXT(vkInstance, &debugReportInfo, NO_ALLOCATOR, &debugReportCallback);
		if (VK_SUCCESS != createDebugReportSuccess)
		{
			mist_Print("Failed to create debug report callback!");
		}

		mist_Print("Created debug report callback!");
	}*/

	mist_Print("Creating vkSurface...");
	// Create the surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = mist_WinInstance;
	surfaceCreateInfo.hwnd = vulkanWindow;

	VkSurfaceKHR vkSurface;
	VkResult createWin32SurfaceSuccess = vkCreateWin32SurfaceKHR(vkInstance, &surfaceCreateInfo, NO_ALLOCATOR, &vkSurface);
	if (VK_SUCCESS != createWin32SurfaceSuccess)
	{
		mist_Print("Error: Failed to create win32 surface!");
		return -1;
	}
	mist_Print("Created vkSurface!");

	struct GPU
	{
		VkPhysicalDevice device;

		mist_ScopedMemory<VkQueueFamilyProperties> queueProperties;
		uint32_t queuePropertyCount;

		mist_ScopedMemory<VkExtensionProperties> extensionProperties;
		uint32_t extensionPropertyCount;

		VkSurfaceCapabilitiesKHR surfaceCapabilities;

		mist_ScopedMemory<VkSurfaceFormatKHR> surfaceFormats;
		uint32_t surfaceFormatCount;

		mist_ScopedMemory<VkPresentModeKHR> presentModes;
		uint32_t presentModeCount;

		VkPhysicalDeviceMemoryProperties memoryProperties;
		VkPhysicalDeviceProperties properties;
	};

	mist_Print("Enumerating physical devices...");
	uint32_t physicalDeviceCount = 0;
	VkResult physicalDeviceCountSuccess = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
	if (VK_SUCCESS != physicalDeviceCountSuccess)
	{
		mist_Print("Error: Failed to get the number of physical devices!");
		return -1;
	}

	mist_ScopedMemory<GPU> GPUs = (GPU*)malloc(sizeof(GPU) * physicalDeviceCount);
	memset(GPUs, 0, sizeof(GPU) * physicalDeviceCount);
	const uint32_t gpuCount = physicalDeviceCount;

	mist_Print("Enumerating device properties...");
	{
		mist_ScopedMemory<VkPhysicalDevice> physicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);

		VkResult physicalDeviceSuccess = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices);
		if (VK_SUCCESS != physicalDeviceSuccess)
		{
			mist_Print("Error: Failed to retrieve the physical devices!");
			return -1;
		}

		for (uint32_t i = 0; i < physicalDeviceCount; i++)
		{
			GPUs[i].device = physicalDevices[i];

			vkGetPhysicalDeviceQueueFamilyProperties(GPUs[i].device, &GPUs[i].queuePropertyCount, nullptr);
			if (0 == GPUs[i].queuePropertyCount)
			{
				mist_Print("Error: Device has no queue families!");
			}

			GPUs[i].queueProperties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * GPUs[i].queuePropertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(GPUs[i].device, &GPUs[i].queuePropertyCount, GPUs[i].queueProperties);
			if (0 == GPUs[i].queuePropertyCount)
			{
				mist_Print("Error: Device has no queue families!");
			}


			VkResult extensionCountResult = vkEnumerateDeviceExtensionProperties(GPUs[i].device, nullptr, &GPUs[i].extensionPropertyCount, nullptr);
			if (VK_SUCCESS != extensionCountResult || 0 == GPUs[i].extensionPropertyCount)
			{
				mist_Print("Error: Device has no extension properties!");
			}

			GPUs[i].extensionProperties = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * GPUs[i].extensionPropertyCount);
			VkResult extensionResult = vkEnumerateDeviceExtensionProperties(GPUs[i].device, nullptr, &GPUs[i].extensionPropertyCount, GPUs[i].extensionProperties);
			if (VK_SUCCESS != extensionResult || 0 == GPUs[i].extensionPropertyCount)
			{
				mist_Print("Error: Device has no extension properties!");
			}


			VkResult surfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPUs[i].device, vkSurface, &GPUs[i].surfaceCapabilities);
			if (VK_SUCCESS != surfaceCapabilitiesResult)
			{
				mist_Print("Error: Failed to get surface capabilities!");
			}


			VkResult surfaceFormatCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(GPUs[i].device, vkSurface, &GPUs[i].surfaceFormatCount, nullptr);
			if (VK_SUCCESS != surfaceFormatCountResult || 0 == GPUs[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no surface formats!");
			}

			GPUs[i].surfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * GPUs[i].surfaceFormatCount);
			VkResult surfaceFormatResult = vkGetPhysicalDeviceSurfaceFormatsKHR(GPUs[i].device, vkSurface, &GPUs[i].surfaceFormatCount, GPUs[i].surfaceFormats);
			if (VK_SUCCESS != surfaceFormatResult || 0 == GPUs[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no surface formats!");
			}


			VkResult presentModeCountResult = vkGetPhysicalDeviceSurfacePresentModesKHR(GPUs[i].device, vkSurface, &GPUs[i].presentModeCount, nullptr);
			if (VK_SUCCESS != presentModeCountResult || 0 == GPUs[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no present modes!");
			}

			GPUs[i].presentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * GPUs[i].presentModeCount);
			VkResult presentModeResult = vkGetPhysicalDeviceSurfacePresentModesKHR(GPUs[i].device, vkSurface, &GPUs[i].presentModeCount, GPUs[i].presentModes);
			if (VK_SUCCESS != presentModeResult || 0 == GPUs[i].presentModeCount)
			{
				mist_Print("Error: Surface has no surface formats!");
			}


			vkGetPhysicalDeviceMemoryProperties(GPUs[i].device, &GPUs[i].memoryProperties);
			vkGetPhysicalDeviceProperties(GPUs[i].device, &GPUs[i].properties);
		}
	}
	mist_Print("Enumerating device properties done!");

	mist_Print("Selecting best physical device...");
	{
		for (uint32_t i = 0; i < gpuCount; i++)
		{
			GPU* gpu = &GPUs[i];

			int32_t graphicsQueue = -1;
			int32_t presentQueu = -1;

			// TODO: keep working
		}
	}
	mist_Print("Best physical device selected!");



	// Game stuff woo!
	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived) 
	{
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

	return 0;
} 