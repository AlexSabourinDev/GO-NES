
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

// Vulkan!
#define VK_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "MistVk.h"
#include "Allocator.h"

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

typedef struct mist_VkBuffer
{
	mist_VkAlloc alloc;
	void*        mappedMem;
	
	VkBuffer     buffer;

} mist_VkBuffer;

mist_VkBuffer mist_VkCreateBuffer(VkDevice device, mist_VkAllocator* allocator, void* data, VkDeviceSize size, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo bufferCreate =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage
	};

	VkBuffer buffer;
	VK_CHECK(vkCreateBuffer(device, &bufferCreate, NO_ALLOCATOR, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	mist_VkAlloc allocation = mist_VkAllocate(allocator, memoryRequirements.size, memoryRequirements.alignment);
	VK_CHECK(vkBindBufferMemory(device, buffer, allocation.memory, allocation.offset));

	void* mappedMemory = mist_VkMapMemory(allocator, allocation);

	memcpy(mappedMemory, data, size);
	return (mist_VkBuffer) { .alloc = allocation, .mappedMem = mappedMemory, .buffer = buffer };
}

void mist_VkFreeBuffer(VkDevice device, mist_VkAllocator* allocator, mist_VkBuffer buffer)
{
	vkDestroyBuffer(device, buffer.buffer, NO_ALLOCATOR);
	mist_VkFree(allocator, buffer.alloc);
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

	#define		vkConfig_AppName "NesEditor"
	#define		vkConfig_EngineName "Mist"

	#define	vkConfig_EnableDebugExtensions true
	#define	vkConfig_EnableValidationLayers true

	const char*		vkConfig_InstanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const char*		vkConfig_InstanceDebugExtensions[] = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	#define			vkConfig_MaxInstanceExtensions ARRAYSIZE(vkConfig_InstanceExtensions) + ARRAYSIZE(vkConfig_InstanceDebugExtensions)

	const char*		vkConfig_DeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	#define			vkConfig_MaxDeviceExtensions ARRAYSIZE(vkConfig_DeviceExtensions)

	const char*		vkConfig_ValidationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	#define			vkConfig_MaxValidationLayers ARRAYSIZE(vkConfig_ValidationLayers)

	#define			vkConfig_BufferCount 2

	HWND vulkanWindow = mist_CreateWindow(winConfig_WindowName, winConfig_IsFullscreen, winConfig_Width, winConfig_Height, mist_WinInstance, WndProc);

	// Vulkan setup! Lets go!
	mist_Print("Vulkan Startup...");

	mist_Print("Creating VkInstance...");

	VkApplicationInfo appInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
		.pApplicationName = vkConfig_AppName,
		.applicationVersion = 1,
		.pEngineName = vkConfig_EngineName,
		.engineVersion = 1,
		.apiVersion = VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION) 
	};
	

	VkInstanceCreateInfo createInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};

	mist_Print("Creating instance layers...");
	const char* instanceExtensions[vkConfig_MaxInstanceExtensions];
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
	VK_CHECK(vkCreateInstance(&createInfo, NO_ALLOCATOR, &vkInstance));

	mist_Print("Created vkInstance!");

	mist_Print("Creating vkSurface...");
	// Create the surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = mist_WinInstance,
		.hwnd = vulkanWindow
	};


	VkSurfaceKHR vkSurface;
	VK_CHECK(vkCreateWin32SurfaceKHR(vkInstance, &surfaceCreateInfo, NO_ALLOCATOR, &vkSurface));
	mist_Print("Created vkSurface!");

	typedef struct GPU
	{
		#define vkConfig_GPU_MaxArraySize 50

		VkPhysicalDevice device;

		VkQueueFamilyProperties queueProperties[vkConfig_GPU_MaxArraySize];
		uint32_t queuePropertyCount;

		VkExtensionProperties extensionProperties[vkConfig_GPU_MaxArraySize];
		uint32_t extensionPropertyCount;

		VkSurfaceCapabilitiesKHR surfaceCapabilities;

		VkSurfaceFormatKHR surfaceFormats[vkConfig_GPU_MaxArraySize];
		uint32_t surfaceFormatCount;

		VkPresentModeKHR presentModes[vkConfig_GPU_MaxArraySize];
		uint32_t presentModeCount;

		VkPhysicalDeviceMemoryProperties memoryProperties;
		VkPhysicalDeviceProperties properties;
	} GPU;

	mist_Print("Enumerating physical devices...");
	uint32_t physicalDeviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, NULL));

	#define vkConfig_MaxGPUs 10

	GPU GPUs[vkConfig_MaxGPUs];
	physicalDeviceCount = min(physicalDeviceCount, vkConfig_MaxGPUs);
	const uint32_t gpuCount = physicalDeviceCount;

	VkPhysicalDevice physicalDevices[vkConfig_MaxGPUs];
	mist_Print("Enumerating device properties...");
	{
		VK_CHECK(vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices));

		for (uint32_t i = 0; i < physicalDeviceCount; i++)
		{
			GPUs[i].device = physicalDevices[i];

			vkGetPhysicalDeviceQueueFamilyProperties(GPUs[i].device, &GPUs[i].queuePropertyCount, NULL);
			if (0 == GPUs[i].queuePropertyCount)
			{
				mist_Print("Error: Device has no queue families!");
				assert(false);
			}
			
			if (GPUs[i].queuePropertyCount > vkConfig_GPU_MaxArraySize)
			{
				mist_Print("Warning: Too many queue properties!");
			}

			GPUs[i].queuePropertyCount = min(vkConfig_GPU_MaxArraySize, GPUs[i].queuePropertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(GPUs[i].device, &GPUs[i].queuePropertyCount, GPUs[i].queueProperties);
			if (0 == GPUs[i].queuePropertyCount)
			{
				mist_Print("Error: Device has no queue families!");
				assert(false);
			}


			VkResult extensionCountResult = vkEnumerateDeviceExtensionProperties(GPUs[i].device, NULL, &GPUs[i].extensionPropertyCount, NULL);
			if (VK_SUCCESS != extensionCountResult || 0 == GPUs[i].extensionPropertyCount)
			{
				mist_Print("Error: Device has no extension properties!");
				assert(false);
			}

			if (GPUs[i].extensionPropertyCount > vkConfig_GPU_MaxArraySize)
			{
				mist_Print("Warning: Too many extension properties!");
			}


			GPUs[i].extensionPropertyCount = min(vkConfig_GPU_MaxArraySize, GPUs[i].extensionPropertyCount);
			VkResult extensionResult = vkEnumerateDeviceExtensionProperties(GPUs[i].device, NULL, &GPUs[i].extensionPropertyCount, GPUs[i].extensionProperties);
			if (VK_SUCCESS != extensionResult || 0 == GPUs[i].extensionPropertyCount)
			{
				mist_Print("Error: Device has no extension properties!");
				assert(false);
			}


			VkResult surfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPUs[i].device, vkSurface, &GPUs[i].surfaceCapabilities);
			if (VK_SUCCESS != surfaceCapabilitiesResult)
			{
				mist_Print("Error: Failed to get surface capabilities!");
				assert(false);
			}


			VkResult surfaceFormatCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(GPUs[i].device, vkSurface, &GPUs[i].surfaceFormatCount, NULL);
			if (VK_SUCCESS != surfaceFormatCountResult || 0 == GPUs[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no surface formats!");
				assert(false);
			}

			if (GPUs[i].surfaceFormatCount > vkConfig_GPU_MaxArraySize)
			{
				mist_Print("Warning: Too many surface formats!");
			}

			GPUs[i].surfaceFormatCount = min(GPUs[i].surfaceFormatCount, vkConfig_GPU_MaxArraySize);
			VkResult surfaceFormatResult = vkGetPhysicalDeviceSurfaceFormatsKHR(GPUs[i].device, vkSurface, &GPUs[i].surfaceFormatCount, GPUs[i].surfaceFormats);
			if (VK_SUCCESS != surfaceFormatResult || 0 == GPUs[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no surface formats!");
				assert(false);
			}


			VkResult presentModeCountResult = vkGetPhysicalDeviceSurfacePresentModesKHR(GPUs[i].device, vkSurface, &GPUs[i].presentModeCount, NULL);
			if (VK_SUCCESS != presentModeCountResult || 0 == GPUs[i].presentModeCount)
			{
				mist_Print("Error: Surface has no present modes!");
				assert(false);
			}

			if (GPUs[i].presentModeCount > vkConfig_GPU_MaxArraySize)
			{
				mist_Print("Warning: Too many present modes!");
			}

			GPUs[i].presentModeCount = min(GPUs[i].presentModeCount, vkConfig_GPU_MaxArraySize);
			VkResult presentModeResult = vkGetPhysicalDeviceSurfacePresentModesKHR(GPUs[i].device, vkSurface, &GPUs[i].presentModeCount, GPUs[i].presentModes);
			if (VK_SUCCESS != presentModeResult || 0 == GPUs[i].presentModeCount)
			{
				mist_Print("Error: Surface has no present modes!");
				assert(false);
			}


			vkGetPhysicalDeviceMemoryProperties(GPUs[i].device, &GPUs[i].memoryProperties);
			vkGetPhysicalDeviceProperties(GPUs[i].device, &GPUs[i].properties);
		}
	}
	mist_Print("Enumerating device properties done!");

	mist_Print("Selecting best physical device...");
	GPU* selectedGPU = NULL;
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
					assert(false);
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

	if (NULL == selectedGPU)
	{
		mist_Print("Error: Failed to select a physical device!");
		assert(false);
		return -1;
	}
	mist_Print("Best physical device selected!");

	mist_Print("Creating logical device...");
	VkDeviceQueueCreateInfo queueCreateInfo[2] = { {0}, {0} };
	int queueCreateInfoCount = 0;

	static const float graphicsQueuePriority = 1.0f;

	VkDeviceQueueCreateInfo createGraphicsQueue = 
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueCount = 1,
		.queueFamilyIndex = selectedGraphicsQueue,
		.pQueuePriorities = &graphicsQueuePriority
	};

	queueCreateInfo[queueCreateInfoCount] = createGraphicsQueue;
	queueCreateInfoCount++;

	if (selectedGraphicsQueue != selectedPresentQueue)
	{
		static const float presentQueuePriority = 1.0f;

		VkDeviceQueueCreateInfo createPresentQueue = 
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueCount = 1,
			.queueFamilyIndex = selectedPresentQueue,
			.pQueuePriorities = &presentQueuePriority
		};

		queueCreateInfo[queueCreateInfoCount] = createPresentQueue;
		queueCreateInfoCount++;
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures = { 0 };


	VkDeviceCreateInfo deviceCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.enabledExtensionCount = vkConfig_MaxDeviceExtensions,
		.queueCreateInfoCount = queueCreateInfoCount,
		.pQueueCreateInfos = queueCreateInfo,
		.pEnabledFeatures = &physicalDeviceFeatures,
		.ppEnabledExtensionNames = vkConfig_DeviceExtensions
	};


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
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VK_CHECK(vkCreateDevice(selectedGPU->device, &deviceCreateInfo, NO_ALLOCATOR, &vkDevice));

	vkGetDeviceQueue(vkDevice, selectedGraphicsQueue, 0, &graphicsQueue);
	vkGetDeviceQueue(vkDevice, selectedPresentQueue, 0, &presentQueue);
	mist_Print("Created logical device!");

	mist_Print("Creating Semaphores...");
	VkSemaphoreCreateInfo semaphoreCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkSemaphore acquireSemaphores[vkConfig_BufferCount];
	VkSemaphore frameCompleteSemaphores[vkConfig_BufferCount];
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VK_CHECK(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NO_ALLOCATOR, &acquireSemaphores[i]));
		VK_CHECK(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NO_ALLOCATOR, &frameCompleteSemaphores[i]));
	}
	mist_Print("Created Semaphores!");

	mist_Print("Creating command buffers...");
	VkCommandPool graphicsCommandPool;

	VkCommandPoolCreateInfo commandPoolCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = selectedGraphicsQueue
	};

	VK_CHECK(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, NO_ALLOCATOR, &graphicsCommandPool));

	VkCommandBuffer graphicsCommandBuffers[vkConfig_BufferCount];

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = vkConfig_BufferCount,
		.commandPool = graphicsCommandPool
	};

	VK_CHECK(vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, graphicsCommandBuffers));
	mist_Print("Created command buffers!");

	mist_Print("Creating fences...");
	VkFence graphicsFences[vkConfig_BufferCount];

	VkFenceCreateInfo fenceCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
	};
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VK_CHECK(vkCreateFence(vkDevice, &fenceCreateInfo, NO_ALLOCATOR, &graphicsFences[i]));
	}
	mist_Print("Created fences!");

	mist_Print("Creating a swapchain...");

	mist_Print("Selecting surface format...");
	if (0 == selectedGPU->surfaceFormatCount)
	{
		mist_Print("Error: No surface formats available!");
		assert(false);
		return -1;
	}

	VkSurfaceFormatKHR surfaceFormat = {0};
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
	VkSurfaceCapabilitiesKHR const* surfaceCapabilities = &selectedGPU->surfaceCapabilities;
	VkExtent2D surfaceExtents = {0};

	if (UINT32_MAX == surfaceCapabilities->currentExtent.width)
	{
		surfaceExtents.width = winConfig_Width;
		surfaceExtents.height = winConfig_Height;
	}
	else
	{
		surfaceExtents = surfaceCapabilities->currentExtent;
	}
	mist_Print("Selected surface extents!");

	VkSwapchainCreateInfoKHR swapchainCreate = 
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.minImageCount = vkConfig_BufferCount,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.surface = vkSurface,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = surfaceExtents,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE
	};
	
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

	VkSwapchainKHR vkSwapchain;
	VK_CHECK(vkCreateSwapchainKHR(vkDevice, &swapchainCreate, NO_ALLOCATOR, &vkSwapchain));
	mist_Print("Created a swapchain!");

	mist_Print("Retrieving swapchain images...");
	#define vkConfig_MaxImageCount 10
	uint32_t imageCount = 0;
	VkImage swapchainPhysicalImages[vkConfig_MaxImageCount];

	VK_CHECK(vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &imageCount, NULL));
	if (imageCount > vkConfig_MaxImageCount)
	{
		mist_Print("Warning: Too many images for the swapchain");
	}

	imageCount = min(vkConfig_MaxImageCount, imageCount);

	VK_CHECK(vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &imageCount, swapchainPhysicalImages));

	if (vkConfig_BufferCount > imageCount)
	{
		mist_Print("Error: Not enough images retrieved from the swapchain!");
		assert(false);
		return -1;
	}
	VkImageView swapchainImages[vkConfig_BufferCount];
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VkImageViewCreateInfo imageViewCreate = 
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,

			.components.r = VK_COMPONENT_SWIZZLE_R,
			.components.g = VK_COMPONENT_SWIZZLE_G,
			.components.b = VK_COMPONENT_SWIZZLE_B,
			.components.a = VK_COMPONENT_SWIZZLE_A,

			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
			.image = swapchainPhysicalImages[i],
			.format = surfaceFormat.format
		};

		VK_CHECK(vkCreateImageView(vkDevice, &imageViewCreate, NO_ALLOCATOR, &swapchainImages[i]));
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

	VkAttachmentDescription colorAttachment = 
	{
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.format = surfaceFormat.format
	};
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

	VkAttachmentReference colorReference = 
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	// TODO: depth
	/*VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

	VkSubpassDescription subpass = 
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorReference
	};

	// subpass.pDepthStencilAttachment = &depthReference; // TODO: depth

	VkRenderPassCreateInfo createRenderPass = 
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = ARRAYSIZE(renderPassAttachments),
		.subpassCount = 1,
		.dependencyCount = 0,
		.pAttachments = renderPassAttachments,
		.pSubpasses = &subpass
	};

	VkRenderPass vkRenderPass;
	VK_CHECK(vkCreateRenderPass(vkDevice, &createRenderPass, NO_ALLOCATOR, &vkRenderPass));
	mist_Print("Created render pass!");

	mist_Print("Creating framebuffers...");
	VkImageView attachments[1];

	// TODO: Add support for depth images
	// attachments[1] = depthImageView;

	VkFramebufferCreateInfo framebufferCreate = 
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.attachmentCount = ARRAYSIZE(attachments),
		.width = winConfig_Width,
		.height = winConfig_Height,
		.layers = 1,
		.renderPass = vkRenderPass,
		.pAttachments = attachments
	};

	VkFramebuffer framebuffers[vkConfig_BufferCount];
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		attachments[0] = swapchainImages[i];
		VK_CHECK(vkCreateFramebuffer(vkDevice, &framebufferCreate, NO_ALLOCATOR, &framebuffers[i]));
	}

	mist_Print("Created framebuffers!");

	mist_VkAllocator vkAllocator;
	uint32_t preferedMemoryIndex = UINT32_MAX;
	VkMemoryType* types = selectedGPU->memoryProperties.memoryTypes;

	VkMemoryPropertyFlags requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VkMemoryPropertyFlags preferedFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	for (uint32_t i = 0; i < selectedGPU->memoryProperties.memoryTypeCount; ++i)
	{
		if ((types[i].propertyFlags & (requiredFlags | preferedFlags)) == (requiredFlags | preferedFlags))
		{
			preferedMemoryIndex = types[i].heapIndex;
			break;
		}
	}

	if (preferedMemoryIndex == UINT32_MAX)
	{
		for (uint32_t i = 0; i < selectedGPU->memoryProperties.memoryTypeCount; ++i)
		{
			if ((types[i].propertyFlags & requiredFlags) == requiredFlags)
			{
				preferedMemoryIndex = i;
				break;
			}
		}
	}

	mist_Print("Initializing allocator...");
	mist_InitVkAllocator(&vkAllocator, vkDevice, 1024 * 1024, preferedMemoryIndex, mist_AllocatorHostVisible);
	mist_Print("Initialized allocator!");

	mist_Print("Initializing shaders...");

	mist_Print("Initializing vertex shader...");
	FILE* vertFile = fopen("../../Assets/Shaders/SPIR-V/default.vspv", "r");

	fseek(vertFile, 0, SEEK_END);
	long vertFileSize = ftell(vertFile);
	fseek(vertFile, 0, SEEK_SET);  //same as rewind(f);

	void* vertShaderSource = malloc(vertFileSize);
	fread(vertShaderSource, vertFileSize, 1, vertFile);
	fclose(vertFile);

	VkShaderModuleCreateInfo createVertShader =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vertFileSize,
		.pCode = (const uint32_t*)vertShaderSource
	};

	VkShaderModule vertShader;
	VK_CHECK(vkCreateShaderModule(vkDevice, &createVertShader, NO_ALLOCATOR, &vertShader));
	mist_Print("Vertex shader initialized!");

	mist_Print("Initializing fragment shader...");
	FILE* fragFile = fopen("../../Assets/Shaders/SPIR-V/default.fspv", "r");

	fseek(fragFile, 0, SEEK_END);
	long fragFileSize = ftell(fragFile);
	fseek(fragFile, 0, SEEK_SET);  //same as rewind(f);

	void* fragShaderSource = malloc(fragFileSize);
	fread(fragShaderSource, fragFileSize, 1, fragFile);
	fclose(fragFile);

	VkShaderModuleCreateInfo createFragShader =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fragFileSize,
		.pCode = (const uint32_t*)fragShaderSource
	};

	VkShaderModule fragShader;
	VK_CHECK(vkCreateShaderModule(vkDevice, &createFragShader, NO_ALLOCATOR, &fragShader));
	mist_Print("Fragment shader initialized!");

	mist_Print("Initialized shaders!");


	mist_Print("Initializing pipeline...");

	mist_Print("Creating descriptor layout...");
	VkDescriptorSetLayoutCreateInfo layoutCreate =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 0,
		.pBindings = NULL
	};

	VkDescriptorSetLayout descriptorSetLayout;
	VK_CHECK(vkCreateDescriptorSetLayout(vkDevice, &layoutCreate, NO_ALLOCATOR, &descriptorSetLayout));
	mist_Print("Created descriptor layout!");

	mist_Print("Creating pipeline layout...");
	VkPipelineLayoutCreateInfo pipelineLayoutCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout
	};

	VkPipelineLayout pipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreate, NO_ALLOCATOR, &pipelineLayout));
	mist_Print("Pipeline layout created!");


	mist_Print("Vertex Input...");

	// Vertex Input
	VkVertexInputBindingDescription inputBinding =
	{
		.stride = sizeof(float) * 3.0f,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription inputAttribute =
	{
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.location = 0,
		.offset = 0
	};

	VkPipelineVertexInputStateCreateInfo vertexInputCreate = 
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &inputBinding,
		.vertexAttributeDescriptionCount = 1,
		.pVertexAttributeDescriptions = &inputAttribute
	};

	mist_Print("Input assembly...");

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	};

	mist_Print("Rasterization...");

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.rasterizerDiscardEnable = VK_FALSE,
		.depthBiasEnable = VK_FALSE,
		.depthClampEnable = VK_FALSE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.0f,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.blendEnable = VK_TRUE,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlendCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE
		/*.front, .back */
	};

	VkPipelineMultisampleStateCreateInfo multisampleCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkPipelineShaderStageCreateInfo vertexStage =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pName = "main",
		.module = vertShader,
		.stage = VK_SHADER_STAGE_VERTEX_BIT
	};

	VkPipelineShaderStageCreateInfo fragStage = 
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pName = "main",
		.module = fragShader,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStage, fragStage };

	VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_LINE_WIDTH };

	VkPipelineDynamicStateCreateInfo dynamicStateCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ARRAYSIZE(dynamicState),
		.pDynamicStates = dynamicState
	};

	VkRect2D scissor =
	{
		{ .x = 0, .y = 0 },
		{ .width = winConfig_Width, .height = winConfig_Height }
	};

	VkViewport viewport =
	{
		.x = 0,
		.y = 0,
		.width = winConfig_Width,
		.height = winConfig_Height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkPipelineViewportStateCreateInfo viewportStateCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkGraphicsPipelineCreateInfo graphicsPipelineCreate =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.layout = pipelineLayout,
		.renderPass = vkRenderPass,
		.pVertexInputState = &vertexInputCreate,
		.pInputAssemblyState = &inputAssemblyCreate,
		.pRasterizationState = &rasterizationCreate,
		.pColorBlendState = &colorBlendCreate,
		.pDepthStencilState = &depthStencilCreate,
		.pMultisampleState = &multisampleCreate,
		.pDynamicState = &dynamicStateCreate,
		.pViewportState = &viewportStateCreate,
		.stageCount = ARRAYSIZE(shaderStages),
		.pStages = shaderStages
	};

	VkPipelineCacheCreateInfo pipelineCacheCreate = 
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
	};

	VkPipelineCache pipelineCache;
	VK_CHECK(vkCreatePipelineCache(vkDevice, &pipelineCacheCreate, NO_ALLOCATOR, &pipelineCache));

	VkPipeline pipeline;
	VK_CHECK(vkCreateGraphicsPipelines(vkDevice, pipelineCache, 1, &graphicsPipelineCreate, NO_ALLOCATOR, &pipeline));

	mist_Print("Pipeline initialized!");


	mist_Print("Creating descriptor pool...");

	#define vkConfig_MaxUniformBufferCount 1000
	#define vkConfig_MaxImageSamples 1000
	VkDescriptorPoolSize descriptorPoolSizes[2] =
	{
		{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = vkConfig_MaxUniformBufferCount },
		{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vkConfig_MaxImageSamples }
	};

	#define vkConfig_MaxDescriptorSets 1000
	VkDescriptorPoolCreateInfo descriptorPoolCreate = 
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = vkConfig_MaxDescriptorSets,
		.poolSizeCount = ARRAYSIZE(descriptorPoolSizes),
		.pPoolSizes = descriptorPoolSizes
	};

	VkDescriptorPool descriptorPools[vkConfig_BufferCount];
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VK_CHECK(vkCreateDescriptorPool(vkDevice, &descriptorPoolCreate, NO_ALLOCATOR, &descriptorPools[i]));
	}

	mist_Print("Created descriptor pool!");

	mist_Print("Creating vertex buffer...");

	float vertices[] =
	{
		-0.5f, 0.0f, 0.0f,
		0.5f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f
	};
	mist_VkBuffer vertexBuffer = mist_VkCreateBuffer(vkDevice, &vkAllocator, vertices, sizeof(float) * ARRAYSIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	mist_Print("Created vertex buffer!");

	VkDescriptorSet descriptorSets[vkConfig_MaxDescriptorSets];
	bool allocatedDescriptorSets = false;

	uint32_t activeDescriptorSet = 0;

	// Game stuff woo!
	uint8_t currentFrame = 0;

	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived)
	{
		// Start the frame
		uint32_t imageIndex = 0;
		VK_CHECK(vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT64_MAX, acquireSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex));

		// Setup the pipeline
		VkDescriptorSetAllocateInfo descriptorSetAlloc =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPools[currentFrame],
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptorSetLayout
		};

		if (allocatedDescriptorSets == false)
		{
			VK_CHECK(vkAllocateDescriptorSets(vkDevice, &descriptorSetAlloc, &descriptorSets[activeDescriptorSet]));
		}
		VkDescriptorSet descriptorSet = descriptorSets[activeDescriptorSet];


		VkCommandBufferBeginInfo beginBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};
		VK_CHECK(vkBeginCommandBuffer(graphicsCommandBuffers[currentFrame], &beginBufferInfo));

		VkMemoryBarrier barrier =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
			.dstAccessMask = 
				VK_ACCESS_INDEX_READ_BIT |
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
				VK_ACCESS_UNIFORM_READ_BIT |
				VK_ACCESS_SHADER_READ_BIT |
				VK_ACCESS_SHADER_WRITE_BIT |
				VK_ACCESS_TRANSFER_READ_BIT |
				VK_ACCESS_TRANSFER_WRITE_BIT
		};
		vkCmdPipelineBarrier(graphicsCommandBuffers[currentFrame],
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 1, &barrier, 0, NULL, 0, NULL);

		#define MIST_FRAME_NUM_CLEARS 1
		VkClearColorValue clearColor = { .float32 = { 0.7f, 0.4f, 0.2f, 1.0f } };
		VkClearValue clearValues[MIST_FRAME_NUM_CLEARS] = 
		{
			{ .color = clearColor }
		};

		VkRenderPassBeginInfo renderPassBeginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = vkRenderPass,
			.framebuffer = framebuffers[currentFrame],
			.renderArea = {.extent = surfaceExtents },
			.clearValueCount = MIST_FRAME_NUM_CLEARS,
			.pClearValues = clearValues
		};

		vkCmdBeginRenderPass(graphicsCommandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(
			graphicsCommandBuffers[currentFrame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout, 0, 1, &descriptorSet,
			0, NULL);

		vkCmdBindPipeline(graphicsCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Draw frame

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(graphicsCommandBuffers[currentFrame], 0, 1, &vertexBuffer.buffer, &offset);
		vkCmdDraw(graphicsCommandBuffers[currentFrame], 3, 1, 0, 0);

		// Finish the frame
		vkCmdEndRenderPass(graphicsCommandBuffers[currentFrame]);
		vkEndCommandBuffer(graphicsCommandBuffers[currentFrame]);

		VkSemaphore* acquire = &acquireSemaphores[currentFrame];
		VkSemaphore* finished = &frameCompleteSemaphores[currentFrame];

		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &graphicsCommandBuffers[currentFrame],
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = acquire,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = finished,
			.pWaitDstStageMask = &dstStageMask
		};

		VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, graphicsFences[currentFrame]));

		// Present the frame
		VK_CHECK(vkWaitForFences(vkDevice, 1, &graphicsFences[currentFrame], VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetFences(vkDevice, 1, &graphicsFences[currentFrame]));
		
		VkPresentInfoKHR presentInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = finished,
			.swapchainCount = 1,
			.pSwapchains = &vkSwapchain,
			.pImageIndices = &imageIndex
		};
		VK_CHECK(vkQueuePresentKHR(presentQueue, &presentInfo));

		currentFrame = (currentFrame + 1) % vkConfig_BufferCount;
		
		activeDescriptorSet++;
		if (activeDescriptorSet >= vkConfig_MaxDescriptorSets)
		{
			allocatedDescriptorSets = true;
			activeDescriptorSet = 0;
		}

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

	mist_VkFreeBuffer(vkDevice, &vkAllocator, vertexBuffer);

	mist_Print("Cleaning up allocator!");
	mist_CleanupVkAllocator(&vkAllocator, vkDevice);
	mist_Print("Cleaned up allocator!");

	vkDestroyPipeline(vkDevice, pipeline, NO_ALLOCATOR);
	vkDestroyPipelineCache(vkDevice, pipelineCache, NO_ALLOCATOR);
	vkDestroyPipelineLayout(vkDevice, pipelineLayout, NO_ALLOCATOR);
	vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, NO_ALLOCATOR);
	vkDestroyShaderModule(vkDevice, fragShader, NO_ALLOCATOR);
	vkDestroyShaderModule(vkDevice, vertShader, NO_ALLOCATOR);

	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		vkDestroyFence(vkDevice, graphicsFences[i], NO_ALLOCATOR);
	}

	vkDestroyCommandPool(vkDevice, graphicsCommandPool, NO_ALLOCATOR);

	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		vkDestroySemaphore(vkDevice, acquireSemaphores[i], NO_ALLOCATOR);
		vkDestroySemaphore(vkDevice, frameCompleteSemaphores[i], NO_ALLOCATOR);
	}

	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		vkDestroyFramebuffer(vkDevice, framebuffers[i], NO_ALLOCATOR);
	}

	vkDestroyRenderPass(vkDevice, vkRenderPass, NO_ALLOCATOR);

	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		vkDestroyImageView(vkDevice, swapchainImages[i], NO_ALLOCATOR);
	}

	vkDestroySwapchainKHR(vkDevice, vkSwapchain, NO_ALLOCATOR);
	vkDestroySurfaceKHR(vkInstance, vkSurface, NO_ALLOCATOR);
	vkDestroyDevice(vkDevice, NO_ALLOCATOR);
	vkDestroyInstance(vkInstance, NO_ALLOCATOR);

	mist_Print("Shutdown!");

	return 0;
} 