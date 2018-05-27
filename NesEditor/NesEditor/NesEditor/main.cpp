
// Standard lib
#include <stdio.h>
#include <assert.h>

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
		if (data != nullptr)
		{
			memset(data, 0, sizeof(Type));
		}
		free(data);
		data = nullptr;
	}

	mist_ScopedMemory& operator=(Type* mem)
	{
		assert(data == nullptr);
		data = mem;
		return *this;
	}

	mist_ScopedMemory(mist_ScopedMemory const&) = delete;
	mist_ScopedMemory& operator=(mist_ScopedMemory const&) = delete;
	mist_ScopedMemory(mist_ScopedMemory&& rhs) = delete;
	mist_ScopedMemory& operator=(mist_ScopedMemory&& rhs) = delete;

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
	const uint32_t   winConfig_Width = 900;
	const uint32_t   winConfig_Height = 700;
	const char* winConfig_WindowName = "Mist Vulkan";

	const char* vkConfig_AppName = "NesEditor";
	const char* vkConfig_EngineName = "Mist";

	const bool  vkConfig_EnableDebugExtensions = true;
	const bool  vkConfig_EnableValidationLayers = true;

	const char*      vkConfig_InstanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const char*      vkConfig_InstanceDebugExtensions[] = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	const uint32_t   vkConfig_MaxInstanceExtensions = ARRAYSIZE(vkConfig_InstanceExtensions) + ARRAYSIZE(vkConfig_InstanceDebugExtensions);

	const char*      vkConfig_DeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const uint32_t   vkConfig_MaxDeviceExtensions = ARRAYSIZE(vkConfig_DeviceExtensions);

	const char*      vkConfig_ValidationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	const uint32_t   vkConfig_MaxValidationLayers = ARRAYSIZE(vkConfig_ValidationLayers);

	const uint32_t   vkConfig_BufferCount = 2;

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

	mist_ScopedMemory<VkPhysicalDevice> physicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
	mist_Print("Enumerating device properties...");
	{
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
				mist_Print("Error: Surface has no present modes!");
			}


			vkGetPhysicalDeviceMemoryProperties(GPUs[i].device, &GPUs[i].memoryProperties);
			vkGetPhysicalDeviceProperties(GPUs[i].device, &GPUs[i].properties);
		}
	}
	mist_Print("Enumerating device properties done!");

	mist_Print("Selecting best physical device...");
	GPU* selectedGPU = nullptr;
	int32_t selectedGraphicsQueue = -1;
	int32_t selectedPresentQueue = -1;

	{
		for (uint32_t i = 0; i < gpuCount; i++)
		{
			GPU* gpu = &GPUs[i];

			int32_t graphicsQueue = -1;
			int32_t presentQueue = -1;

			if (0 == gpu->queuePropertyCount)
			{
				continue;
			}

			if (0 == gpu->presentModeCount)
			{
				continue;
			}

			for (uint32_t fIdx = 0; fIdx < gpu->queuePropertyCount; fIdx++)
			{
				if (gpu->queueProperties[fIdx].queueCount == 0)
				{
					continue;
				}

				if (gpu->queueProperties[fIdx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					graphicsQueue = (int32_t)fIdx;
					break;
				}
			}

			for (uint32_t fIdx = 0; fIdx < gpu->queuePropertyCount; fIdx++)
			{
				if (gpu->queueProperties[fIdx].queueCount == 0)
				{
					continue;
				}

				VkBool32 supportsPresent = VK_FALSE;
				VkResult supportCall = vkGetPhysicalDeviceSurfaceSupportKHR(gpu->device, fIdx, vkSurface, &supportsPresent);
				if (VK_SUCCESS != supportCall)
				{
					mist_Print("Error: Failed to check if the physical device supports KHR!");
					continue;
				}

				if (supportsPresent)
				{
					presentQueue = (int32_t)fIdx;
					break;
				}
			}

			// Did we find a device supporting both graphics and present.
			if (graphicsQueue >= 0 && presentQueue >= 0)
			{
				selectedGraphicsQueue = graphicsQueue;
				selectedPresentQueue = presentQueue;
				selectedGPU = gpu;
				break;
			}
		}
	}

	if (nullptr == selectedGPU)
	{
		mist_Print("Error: Failed to select a physical device!");
		return -1;
	}
	mist_Print("Best physical device selected!");

	mist_Print("Creating logical device...");
	VkDeviceQueueCreateInfo queueCreateInfo[2] = {};
	int queueCreateInfoCount = 0;

	static const float graphicsQueuePriority = 1.0f;

	VkDeviceQueueCreateInfo createGraphicsQueue = {};
	createGraphicsQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	createGraphicsQueue.queueFamilyIndex = selectedGraphicsQueue;
	createGraphicsQueue.queueCount = 1;
	createGraphicsQueue.pQueuePriorities = &graphicsQueuePriority;
	queueCreateInfo[queueCreateInfoCount] = createGraphicsQueue;
	queueCreateInfoCount++;

	if (selectedGraphicsQueue != selectedPresentQueue)
	{
		static const float presentQueuePriority = 1.0f;

		VkDeviceQueueCreateInfo createPresentQueue = {};
		createPresentQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createPresentQueue.queueFamilyIndex = selectedPresentQueue;
		createPresentQueue.queueCount = 1;
		createPresentQueue.pQueuePriorities = &presentQueuePriority;
		queueCreateInfo[queueCreateInfoCount] = createGraphicsQueue;
		queueCreateInfoCount++;
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = vkConfig_MaxDeviceExtensions;
	deviceCreateInfo.ppEnabledExtensionNames = vkConfig_DeviceExtensions;

	if (vkConfig_EnableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = validationLayerCount;
		deviceCreateInfo.ppEnabledLayerNames = validationLayers;
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	VkDevice vkDevice;
	VkQueue graphicsQueue = {};
	VkQueue presentQueue = {};

	VkResult createdLogicalDevice = vkCreateDevice(selectedGPU->device, &deviceCreateInfo, NO_ALLOCATOR, &vkDevice);
	if (VK_SUCCESS != createdLogicalDevice)
	{
		mist_Print("Error: Failed to create a logical device!");
		return -1;
	}

	vkGetDeviceQueue(vkDevice, selectedGraphicsQueue, 0, &graphicsQueue);
	vkGetDeviceQueue(vkDevice, selectedPresentQueue, 0, &presentQueue);
	mist_Print("Created logical device!");

	mist_Print("Creating Semaphores...");
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore acquireSemaphores[vkConfig_BufferCount] = {};
	VkSemaphore frameCompleteSemaphores[vkConfig_BufferCount] = {};
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VkResult acquireSemaphoreCreation = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NO_ALLOCATOR, &acquireSemaphores[i]);
		if (VK_SUCCESS != acquireSemaphoreCreation)
		{
			mist_Print("Error: Failed to create an acquire semaphore!");
			return -1;
		}

		VkResult frameSemaphoreCreation = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NO_ALLOCATOR, &frameCompleteSemaphores[i]);
		if (VK_SUCCESS != frameSemaphoreCreation)
		{
			mist_Print("Error: Failed to create a frame semaphore!");
			return -1;
		}
	}
	mist_Print("Created Semaphores!");

	mist_Print("Creating command buffers...");
	VkCommandPool graphicsCommandPool = {};

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = selectedGraphicsQueue;

	VkResult createdCommandPool = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, NO_ALLOCATOR, &graphicsCommandPool);
	if (VK_SUCCESS != createdCommandPool)
	{
		mist_Print("Error: Failed to create the command pool!");
		return -1;
	}

	VkCommandBuffer graphicsCommandBuffers[vkConfig_BufferCount] = {};

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = graphicsCommandPool;
	commandBufferAllocateInfo.commandBufferCount = vkConfig_BufferCount;

	VkResult allocatedCommandBuffer = vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, graphicsCommandBuffers);
	if (VK_SUCCESS != allocatedCommandBuffer)
	{
		mist_Print("Error: Failed to allocate the command buffer!");
		return -1;
	}
	mist_Print("Created command buffers!");

	mist_Print("Creating fences...");
	VkFence graphicsFences[vkConfig_BufferCount] = {};

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VkResult createdFence = vkCreateFence(vkDevice, &fenceCreateInfo, NO_ALLOCATOR, &graphicsFences[i]);
		if (VK_SUCCESS != createdFence)
		{
			mist_Print("Error: Failed to create fence!");
			return -1;
		}
	}
	mist_Print("Created fences!");

	mist_Print("Creating a swapchain...");

	mist_Print("Selecting surface format...");
	if (0 == selectedGPU->surfaceFormatCount)
	{
		mist_Print("Error: No surface formats available!");
		return -1;
	}

	VkSurfaceFormatKHR surfaceFormat = {};
	if (1 == selectedGPU->surfaceFormatCount && VK_FORMAT_UNDEFINED == selectedGPU->surfaceFormats[0].format)
	{
		surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	else
	{
		surfaceFormat = selectedGPU->surfaceFormats[0];
		for (uint32_t i = 0; i < selectedGPU->surfaceFormatCount; i++)
		{
			if (VK_FORMAT_B8G8R8A8_UNORM == selectedGPU->surfaceFormats[i].format && VK_COLORSPACE_SRGB_NONLINEAR_KHR == selectedGPU->surfaceFormats[i].colorSpace)
			{
				surfaceFormat = selectedGPU->surfaceFormats[i];
				break;
			}
		}
	}
	mist_Print("Selected surface format!");

	mist_Print("Selecting present mode...");
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < selectedGPU->presentModeCount; i++)
	{
		if (VK_PRESENT_MODE_MAILBOX_KHR == selectedGPU->presentModes[i])
		{
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	mist_Print("Selected present mode!");

	mist_Print("Selecting surface extents...");
	VkSurfaceCapabilitiesKHR const& surfaceCapabilities = selectedGPU->surfaceCapabilities;
	VkExtent2D surfaceExtents = {};

	if (-1 == surfaceCapabilities.currentExtent.width)
	{
		surfaceExtents.width = winConfig_Width;
		surfaceExtents.height = winConfig_Height;
	}
	else
	{
		surfaceExtents = surfaceCapabilities.currentExtent;
	}
	mist_Print("Selected surface extents!");

	VkSwapchainCreateInfoKHR swapchainCreate = {};
	swapchainCreate.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreate.surface = vkSurface;

	swapchainCreate.minImageCount = vkConfig_BufferCount;

	swapchainCreate.imageFormat = surfaceFormat.format;
	swapchainCreate.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreate.imageExtent = surfaceExtents;
	swapchainCreate.imageArrayLayers = 1;

	swapchainCreate.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t swapchainShareIndices[2];
	if (selectedGraphicsQueue != selectedPresentQueue)
	{
		swapchainShareIndices[0] = selectedGraphicsQueue;
		swapchainShareIndices[1] = selectedPresentQueue;

		swapchainCreate.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreate.queueFamilyIndexCount = 2;
		swapchainCreate.pQueueFamilyIndices = swapchainShareIndices;
	}
	else
	{
		swapchainCreate.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapchainCreate.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreate.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreate.presentMode = presentMode;

	swapchainCreate.clipped = VK_TRUE;

	VkSwapchainKHR vkSwapchain;
	VkResult createdSwapchain = vkCreateSwapchainKHR(vkDevice, &swapchainCreate, NO_ALLOCATOR, &vkSwapchain);
	if (VK_SUCCESS != createdSwapchain)
	{
		mist_Print("Error: Failed to create swapchain!");
		return -1;
	}
	mist_Print("Created a swapchain!");

	mist_Print("Retrieving swapchain images...");
	uint32_t imageCount = 0;

	VkResult retrieveImageCount = vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &imageCount, nullptr);
	if (VK_SUCCESS != retrieveImageCount)
	{
		mist_Print("Error: Failed to retrieve the image count from the swapchain!");
		return -1;
	}

	mist_ScopedMemory<VkImage> swapchainPhysicalImages = (VkImage*)malloc(sizeof(VkImage) * imageCount);
	VkResult retrieveSwapchainImages = vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &imageCount, swapchainPhysicalImages);
	if (VK_SUCCESS != retrieveSwapchainImages)
	{
		mist_Print("Error: Failed to retrieve the images from the swapchain!");
		return -1;
	}

	if (vkConfig_BufferCount > imageCount)
	{
		mist_Print("Error: Not enough images retrieved from the swapchain!");
		return -1;
	}
	VkImageView swapchainImages[vkConfig_BufferCount] = {};
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VkImageViewCreateInfo imageViewCreate = {};
		imageViewCreate.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreate.image = swapchainPhysicalImages[i];
		imageViewCreate.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreate.format = surfaceFormat.format;

		imageViewCreate.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreate.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreate.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreate.components.a = VK_COMPONENT_SWIZZLE_A;

		imageViewCreate.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreate.subresourceRange.baseMipLevel = 0;
		imageViewCreate.subresourceRange.levelCount = 1;
		imageViewCreate.subresourceRange.baseArrayLayer = 0;
		imageViewCreate.subresourceRange.layerCount = 1;

		VkResult createdImageView = vkCreateImageView(vkDevice, &imageViewCreate, NO_ALLOCATOR, &swapchainImages[i]);
		if (VK_SUCCESS != createdImageView)
		{
			mist_Print("Error: Failed to create an image view!");
			return -1;
		}
	}
	mist_Print("Retrieved swapchain images!");

	// TODO: Add depth when we need it.
	//mist_Print("Selecting depth format...");
	//const VkFormat preferedFormats[] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	//const VkFormatFeatureFlags preferedFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	//VkFormat selectedDepthFormat = VK_FORMAT_UNDEFINED;
	//for (uint32_t i = 0; i < ARRAYSIZE(preferedFormats); i++)
	//{
	//	VkFormatProperties formatProperties;
	//	vkGetPhysicalDeviceFormatProperties(selectedGPU->device, preferedFormats[i], &formatProperties);

	//	if ((formatProperties.optimalTilingFeatures & preferedFeatures) == preferedFeatures)
	//	{
	//		selectedDepthFormat = preferedFormats[i];
	//		break;
	//	}

	//	/*TODO: Actually create the image*/
	//}
	//mist_Print("Selected depth format!");

	mist_Print("Creating render pass...");

	VkAttachmentDescription renderPassAttachments[1];

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	renderPassAttachments[0] = colorAttachment;

	// TODO: Add depth when we can create images
	/*VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = selectedDepthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	renderPassAttachments[1] = depthAttachment;*/

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// TODO: depth
	/*VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	// subpass.pDepthStencilAttachment = &depthReference; // TODO: depth

	VkRenderPassCreateInfo createRenderPass = {};
	createRenderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createRenderPass.attachmentCount = ARRAYSIZE(renderPassAttachments);
	createRenderPass.pAttachments = renderPassAttachments;
	createRenderPass.subpassCount = 1;
	createRenderPass.pSubpasses = &subpass;
	createRenderPass.dependencyCount = 0;

	VkRenderPass vkRenderPass;
	VkResult renderPassCreated = vkCreateRenderPass(vkDevice, &createRenderPass, NO_ALLOCATOR, &vkRenderPass);
	if (VK_SUCCESS != renderPassCreated)
	{
		mist_Print("Error: Failed to create a render pass!");
		return -1;
	}
	mist_Print("Created render pass!");

	mist_Print("Creating framebuffers...");
	VkImageView attachments[1];

	// TODO: Add support for depth images
	// attachments[1] = depthImageView;

	VkFramebufferCreateInfo framebufferCreate = {};
	framebufferCreate.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreate.renderPass = vkRenderPass;
	framebufferCreate.attachmentCount = ARRAYSIZE(attachments);
	framebufferCreate.pAttachments = attachments;
	framebufferCreate.width = winConfig_Width;
	framebufferCreate.height = winConfig_Height;
	framebufferCreate.layers = 1;

	VkFramebuffer framebuffers[vkConfig_BufferCount];
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		attachments[0] = swapchainImages[i];
		VkResult framebufferCreated = vkCreateFramebuffer(vkDevice, &framebufferCreate, NO_ALLOCATOR, &framebuffers[i]);
		if (VK_SUCCESS != framebufferCreated)
		{
			mist_Print("Error: Failed to create framebuffer!");
			return -1;
		}
	}

	mist_Print("Created framebuffers!");

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