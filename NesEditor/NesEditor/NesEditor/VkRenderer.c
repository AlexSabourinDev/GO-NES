#include "VkRenderer.h"

#include <stdio.h>

// Vulkan!
#define VK_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "MistVk.h"
#include "VkAllocator.h"

// Config!
#define		vkConfig_AppName "NesEditor"
#define		vkConfig_EngineName "Mist"

#define		vkConfig_EnableDebugExtensions true
#define		vkConfig_EnableValidationLayers true

const char*		vkConfig_InstanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
const char*		vkConfig_InstanceDebugExtensions[] = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
#define			vkConfig_MaxInstanceExtensions ARRAYSIZE(vkConfig_InstanceExtensions) + ARRAYSIZE(vkConfig_InstanceDebugExtensions)

const char*		vkConfig_DeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#define			vkConfig_MaxDeviceExtensions ARRAYSIZE(vkConfig_DeviceExtensions)

const char*		vkConfig_ValidationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
#define			vkConfig_MaxValidationLayers ARRAYSIZE(vkConfig_ValidationLayers)

#define			vkConfig_BufferCount 2
#define			vkConfig_MaxPhysicalDevices 10
#define			vkConfig_PhysicalDevice_MaxArraySize 50
#define			vkConfig_MaxDescriptorSets 1000

#define			vkConfig_VertexBinding 0
#define			vkConfig_InstanceBinding 1

#define			vkConfig_MaxInstanceCount 10000


extern void mist_Print(const char* message);

typedef struct PhysicalDevice
{
	VkPhysicalDevice device;

	VkQueueFamilyProperties queueProperties[vkConfig_PhysicalDevice_MaxArraySize];
	uint32_t queuePropertyCount;

	VkExtensionProperties extensionProperties[vkConfig_PhysicalDevice_MaxArraySize];
	uint32_t extensionPropertyCount;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;

	VkSurfaceFormatKHR surfaceFormats[vkConfig_PhysicalDevice_MaxArraySize];
	uint32_t surfaceFormatCount;

	VkPresentModeKHR presentModes[vkConfig_PhysicalDevice_MaxArraySize];
	uint32_t presentModeCount;

	VkPhysicalDeviceMemoryProperties memoryProperties;
	VkPhysicalDeviceProperties properties;
} PhysicalDevice;

// Globals!
VkInstance g_VkInstance;
VkSurfaceKHR g_VkSurface;
VkDevice g_VkDevice;
VkSwapchainKHR g_VkSwapchain;
VkRenderPass g_VkRenderPass;

VkQueue g_VkGraphicsQueue;
VkQueue g_VkPresentQueue;

uint32_t g_VkGraphicsQueueIdx;
uint32_t g_VkPresentQueueIdx;

PhysicalDevice g_PhysicalDevices[vkConfig_MaxPhysicalDevices];
uint32_t g_PhysicalDeviceCount;

const char* g_ValidationLayers[vkConfig_MaxValidationLayers];
int32_t g_ValidationLayerCount = 0;

VkSemaphore g_AcquireSemaphores[vkConfig_BufferCount];
VkSemaphore g_FrameCompleteSemaphores[vkConfig_BufferCount];

VkCommandPool g_GraphicsCommandPool;
VkCommandBuffer g_GraphicsCommandBuffers[vkConfig_BufferCount];

VkFence g_GraphicsFences[vkConfig_BufferCount];
VkFramebuffer g_VkFramebuffers[vkConfig_BufferCount];
VkImageView g_VkSwapchainImages[vkConfig_BufferCount];

VkExtent2D g_VkSurfaceExtents;

VkPipelineLayout g_VkPipelineLayout;
VkPipelineCache g_VkPipelineCache;
VkPipeline g_VkPipeline;

VkDescriptorSetLayout g_VkDescriptorSetLayout;
VkDescriptorPool g_VkDescriptorPools[vkConfig_BufferCount];

mist_VkAllocator g_VkAllocator;

typedef struct mist_VkVertex
{
	mist_Vec2 position;
} mist_VkVertex;

typedef struct mist_VkInstance
{
	mist_Vec2 position;
	mist_Vec2 scale;
} mist_VkInstance;

typedef struct mist_VkBuffer
{
	mist_VkAlloc alloc;
	void*        mappedMem;
	VkBuffer     buffer;
} mist_VkBuffer;

typedef enum mist_VkShader
{
	VkShader_DefaultVert = 0,
	VkShader_DefaultFrag,
	VkShader_Count
} mist_VkShader;
VkShaderModule g_VkShaders[VkShader_Count];

mist_VkBuffer g_VkMeshes[VkMesh_Count];
mist_VkBuffer g_VkInstances[VkMesh_Count];

mist_VkBuffer g_VkIndirectDrawBuffer;

VkInstance CreateVkInstance()
{
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

	if (vkConfig_EnableValidationLayers)
	{
		mist_Print("Adding validation layers...");
		for (uint32_t i = 0; i < ARRAYSIZE(vkConfig_ValidationLayers); i++)
		{
			mist_Print(vkConfig_ValidationLayers[i]);

			g_ValidationLayers[i] = vkConfig_ValidationLayers[i];
		}
		g_ValidationLayerCount += ARRAYSIZE(vkConfig_ValidationLayers);

		mist_Print("Adding validation layers done!");
	}

	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = instanceExtensions;
	createInfo.enabledLayerCount = g_ValidationLayerCount;
	createInfo.ppEnabledLayerNames = g_ValidationLayers;

	VkInstance vkInstance;
	VK_CHECK(vkCreateInstance(&createInfo, NO_ALLOCATOR, &vkInstance));

	mist_Print("Created vkInstance!");
	return vkInstance;
}

void DestroyVkInstance(
	VkInstance instance)
{
	vkDestroyInstance(instance, NO_ALLOCATOR);
}

VkSurfaceKHR CreateVkSurface(
	HINSTANCE appInstance, 
	HWND window, 
	VkInstance instance)
{
	mist_Print("Creating vkSurface...");
	// Create the surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = appInstance,
		.hwnd = window
	};

	VkSurfaceKHR vkSurface;
	VK_CHECK(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, NO_ALLOCATOR, &vkSurface));
	mist_Print("Created vkSurface!");

	return vkSurface;
}

void DestroyVkSurface(
	VkInstance instance,
	VkSurfaceKHR surface)
{
	vkDestroySurfaceKHR(instance, surface, NO_ALLOCATOR);
}

void EnumeratePhysicalDevices(
	VkInstance instance, 
	VkSurfaceKHR surface)
{
	mist_Print("Enumerating physical devices...");
	uint32_t physicalDeviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL));

	physicalDeviceCount = min(physicalDeviceCount, vkConfig_MaxPhysicalDevices);
	g_PhysicalDeviceCount = physicalDeviceCount;

	VkPhysicalDevice physicalDevices[vkConfig_MaxPhysicalDevices];
	mist_Print("Enumerating device properties...");
	{
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

		for (uint32_t i = 0; i < physicalDeviceCount; i++)
		{
			g_PhysicalDevices[i].device = physicalDevices[i];

			vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevices[i].device, &g_PhysicalDevices[i].queuePropertyCount, NULL);
			if (0 == g_PhysicalDevices[i].queuePropertyCount)
			{
				mist_Print("Error: Device has no queue families!");
				assert(false);
			}

			if (g_PhysicalDevices[i].queuePropertyCount > vkConfig_PhysicalDevice_MaxArraySize)
			{
				mist_Print("Warning: Too many queue properties!");
			}

			g_PhysicalDevices[i].queuePropertyCount = min(vkConfig_PhysicalDevice_MaxArraySize, g_PhysicalDevices[i].queuePropertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevices[i].device, &g_PhysicalDevices[i].queuePropertyCount, g_PhysicalDevices[i].queueProperties);
			if (0 == g_PhysicalDevices[i].queuePropertyCount)
			{
				mist_Print("Error: Device has no queue families!");
				assert(false);
			}


			VkResult extensionCountResult = vkEnumerateDeviceExtensionProperties(g_PhysicalDevices[i].device, NULL, &g_PhysicalDevices[i].extensionPropertyCount, NULL);
			if (VK_SUCCESS != extensionCountResult || 0 == g_PhysicalDevices[i].extensionPropertyCount)
			{
				mist_Print("Error: Device has no extension properties!");
				assert(false);
			}

			if (g_PhysicalDevices[i].extensionPropertyCount > vkConfig_PhysicalDevice_MaxArraySize)
			{
				mist_Print("Warning: Too many extension properties!");
			}


			g_PhysicalDevices[i].extensionPropertyCount = min(vkConfig_PhysicalDevice_MaxArraySize, g_PhysicalDevices[i].extensionPropertyCount);
			VkResult extensionResult = vkEnumerateDeviceExtensionProperties(g_PhysicalDevices[i].device, NULL, &g_PhysicalDevices[i].extensionPropertyCount, g_PhysicalDevices[i].extensionProperties);
			if (VK_SUCCESS != extensionResult || 0 == g_PhysicalDevices[i].extensionPropertyCount)
			{
				mist_Print("Error: Device has no extension properties!");
				assert(false);
			}


			VkResult surfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevices[i].device, surface, &g_PhysicalDevices[i].surfaceCapabilities);
			if (VK_SUCCESS != surfaceCapabilitiesResult)
			{
				mist_Print("Error: Failed to get surface capabilities!");
				assert(false);
			}


			VkResult surfaceFormatCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevices[i].device, surface, &g_PhysicalDevices[i].surfaceFormatCount, NULL);
			if (VK_SUCCESS != surfaceFormatCountResult || 0 == g_PhysicalDevices[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no surface formats!");
				assert(false);
			}

			if (g_PhysicalDevices[i].surfaceFormatCount > vkConfig_PhysicalDevice_MaxArraySize)
			{
				mist_Print("Warning: Too many surface formats!");
			}

			g_PhysicalDevices[i].surfaceFormatCount = min(g_PhysicalDevices[i].surfaceFormatCount, vkConfig_PhysicalDevice_MaxArraySize);
			VkResult surfaceFormatResult = vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevices[i].device, surface, &g_PhysicalDevices[i].surfaceFormatCount, g_PhysicalDevices[i].surfaceFormats);
			if (VK_SUCCESS != surfaceFormatResult || 0 == g_PhysicalDevices[i].surfaceFormatCount)
			{
				mist_Print("Error: Surface has no surface formats!");
				assert(false);
			}


			VkResult presentModeCountResult = vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevices[i].device, surface, &g_PhysicalDevices[i].presentModeCount, NULL);
			if (VK_SUCCESS != presentModeCountResult || 0 == g_PhysicalDevices[i].presentModeCount)
			{
				mist_Print("Error: Surface has no present modes!");
				assert(false);
			}

			if (g_PhysicalDevices[i].presentModeCount > vkConfig_PhysicalDevice_MaxArraySize)
			{
				mist_Print("Warning: Too many present modes!");
			}

			g_PhysicalDevices[i].presentModeCount = min(g_PhysicalDevices[i].presentModeCount, vkConfig_PhysicalDevice_MaxArraySize);
			VkResult presentModeResult = vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevices[i].device, surface, &g_PhysicalDevices[i].presentModeCount, g_PhysicalDevices[i].presentModes);
			if (VK_SUCCESS != presentModeResult || 0 == g_PhysicalDevices[i].presentModeCount)
			{
				mist_Print("Error: Surface has no present modes!");
				assert(false);
			}


			vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevices[i].device, &g_PhysicalDevices[i].memoryProperties);
			vkGetPhysicalDeviceProperties(g_PhysicalDevices[i].device, &g_PhysicalDevices[i].properties);
		}
	}
	mist_Print("Enumerating device properties done!");
}

void SelectBestPhysicalDevice(
	VkSurfaceKHR surface, 
	PhysicalDevice** selectedGPU, 
	uint32_t* selectedGraphicsQueue, 
	uint32_t* selectedPresentQueue)
{
	mist_Print("Selecting best physical device...");
	*selectedGPU = NULL;
	*selectedGraphicsQueue = UINT32_MAX;
	*selectedPresentQueue = UINT32_MAX;

	{
		for (uint32_t i = 0; i < g_PhysicalDeviceCount; i++)
		{
			PhysicalDevice* physicalDevice = &g_PhysicalDevices[i];

			int32_t graphicsQueue = -1;
			int32_t presentQueue = -1;

			if (0 == physicalDevice->queuePropertyCount)
			{
				continue;
			}

			if (0 == physicalDevice->presentModeCount)
			{
				continue;
			}

			for (uint32_t fIdx = 0; fIdx < physicalDevice->queuePropertyCount; fIdx++)
			{
				if (physicalDevice->queueProperties[fIdx].queueCount == 0)
				{
					continue;
				}

				if (physicalDevice->queueProperties[fIdx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					graphicsQueue = (int32_t)fIdx;
					break;
				}
			}

			for (uint32_t fIdx = 0; fIdx < physicalDevice->queuePropertyCount; fIdx++)
			{
				if (physicalDevice->queueProperties[fIdx].queueCount == 0)
				{
					continue;
				}

				VkBool32 supportsPresent = VK_FALSE;
				VkResult supportCall = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->device, fIdx, surface, &supportsPresent);
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
				*selectedGraphicsQueue = graphicsQueue;
				*selectedPresentQueue = presentQueue;
				*selectedGPU = physicalDevice;
				break;
			}
		}
	}

	if (NULL == *selectedGPU)
	{
		mist_Print("Error: Failed to select a physical device!");
		assert(false);
		return;
	}
	mist_Print("Best physical device selected!");
}

void CreateVkLogicDevice(
	VkPhysicalDevice physicalDevice, 
	int32_t graphicsQueueIdx, 
	int32_t presentQueueIdx, 
	VkDevice* device, 
	VkQueue* graphicsQueue, 
	VkQueue* presentQueue)
{
	mist_Print("Creating logical device...");
	VkDeviceQueueCreateInfo queueCreateInfo[2] = { { 0 },{ 0 } };
	int queueCreateInfoCount = 0;

	static const float graphicsQueuePriority = 1.0f;

	VkDeviceQueueCreateInfo createGraphicsQueue =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueCount = 1,
		.queueFamilyIndex = graphicsQueueIdx,
		.pQueuePriorities = &graphicsQueuePriority
	};

	queueCreateInfo[queueCreateInfoCount] = createGraphicsQueue;
	queueCreateInfoCount++;

	if (graphicsQueueIdx != presentQueueIdx)
	{
		static const float presentQueuePriority = 1.0f;

		VkDeviceQueueCreateInfo createPresentQueue =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueCount = 1,
			.queueFamilyIndex = presentQueueIdx,
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
		deviceCreateInfo.enabledLayerCount = g_ValidationLayerCount;
		deviceCreateInfo.ppEnabledLayerNames = g_ValidationLayers;
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, NO_ALLOCATOR, device));

	vkGetDeviceQueue(*device, graphicsQueueIdx, 0, graphicsQueue);
	vkGetDeviceQueue(*device, presentQueueIdx, 0, presentQueue);
	mist_Print("Created logical device!");
}

void DestroyVkLogicDevice(
	VkDevice device)
{
	vkDestroyDevice(device, NO_ALLOCATOR);
}

void CreateVkSemaphores(
	VkDevice device, 
	VkSemaphore* semaphores, 
	uint32_t semaphoreCount)
{
	mist_Print("Creating Semaphores...");
	VkSemaphoreCreateInfo semaphoreCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	for (uint32_t i = 0; i < semaphoreCount; i++)
	{
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, NO_ALLOCATOR, &semaphores[i]));
	}
	mist_Print("Created Semaphores!");
}

void DestroyVkSemaphores(
	VkDevice device,
	VkSemaphore* semaphores,
	uint32_t semaphoreCount)
{
	for (uint32_t i = 0; i < semaphoreCount; i++)
	{
		vkDestroySemaphore(device, semaphores[i], NO_ALLOCATOR);
	}
}

void CreateVkCommandBuffers(
	VkDevice device, 
	uint32_t queue, 
	VkCommandPool* pool, 
	VkCommandBuffer* commandBuffers, 
	uint32_t bufferCount)
{
	mist_Print("Creating command buffers...");

	VkCommandPoolCreateInfo commandPoolCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queue
	};

	VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, NO_ALLOCATOR, pool));

	VkCommandBufferAllocateInfo commandBufferAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = bufferCount,
		.commandPool = *pool
	};

	VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers));
	mist_Print("Created command buffers!");
}

void DestroyVkCommandBuffers(
	VkDevice device,
	VkCommandPool pool)
{
	vkDestroyCommandPool(device, pool, NO_ALLOCATOR);
}

void CreateVkFences(
	VkDevice device, 
	VkFence* fences, 
	uint32_t fenceCount)
{
	mist_Print("Creating fences...");

	VkFenceCreateInfo fenceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
	};
	for (uint32_t i = 0; i < fenceCount; i++)
	{
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, NO_ALLOCATOR, &fences[i]));
	}
	mist_Print("Created fences!");
}

void DestroyVkFences(
	VkDevice device,
	VkFence* fences,
	uint32_t fenceCount)
{
	for (uint32_t i = 0; i < fenceCount; i++)
	{
		vkDestroyFence(device, fences[i], NO_ALLOCATOR);
	}
}

VkSurfaceFormatKHR SelectVkSurfaceFormat(
	PhysicalDevice* physicalDevice)
{
	mist_Print("Selecting surface format...");
	if (0 == physicalDevice->surfaceFormatCount)
	{
		mist_Print("Error: No surface formats available!");
		assert(false);
	}

	VkSurfaceFormatKHR surfaceFormat = { 0 };
	if (1 == physicalDevice->surfaceFormatCount && VK_FORMAT_UNDEFINED == physicalDevice->surfaceFormats[0].format)
	{
		surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
		surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	else
	{
		surfaceFormat = physicalDevice->surfaceFormats[0];
		for (uint32_t i = 0; i < physicalDevice->surfaceFormatCount; i++)
		{
			if (VK_FORMAT_B8G8R8A8_UNORM == physicalDevice->surfaceFormats[i].format && VK_COLORSPACE_SRGB_NONLINEAR_KHR == physicalDevice->surfaceFormats[i].colorSpace)
			{
				surfaceFormat = physicalDevice->surfaceFormats[i];
				break;
			}
		}
	}
	mist_Print("Selected surface format!");

	return surfaceFormat;
}

VkExtent2D CreateSurfaceExtents(PhysicalDevice* device, uint32_t width, uint32_t height)
{
	mist_Print("Selecting surface extents...");
	VkSurfaceCapabilitiesKHR const* surfaceCapabilities = &device->surfaceCapabilities;
	VkExtent2D surfaceExtents = { 0 };

	if (UINT32_MAX == surfaceCapabilities->currentExtent.width)
	{
		surfaceExtents.width = width;
		surfaceExtents.height = height;
	}
	else
	{
		surfaceExtents = surfaceCapabilities->currentExtent;
	}
	mist_Print("Selected surface extents!");

	return surfaceExtents;
}

VkSwapchainKHR CreateVkSwapchain(
	PhysicalDevice* physicalDevice, 
	VkDevice device, 
	VkSurfaceKHR surface, 
	VkSurfaceFormatKHR surfaceFormat, 
	VkExtent2D surfaceExtents, 
	uint32_t graphicsQueue, 
	uint32_t presentQueue,
	uint32_t framebufferCount)
{
	mist_Print("Creating a swapchain...");

	mist_Print("Selecting present mode...");
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < physicalDevice->presentModeCount; i++)
	{
		if (VK_PRESENT_MODE_MAILBOX_KHR == physicalDevice->presentModes[i])
		{
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	mist_Print("Selected present mode!");

	VkSwapchainCreateInfoKHR swapchainCreate =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.minImageCount = framebufferCount,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.surface = surface,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = surfaceExtents,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE
	};

	uint32_t swapchainShareIndices[2];
	if (graphicsQueue != presentQueue)
	{
		swapchainShareIndices[0] = graphicsQueue;
		swapchainShareIndices[1] = presentQueue;

		swapchainCreate.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreate.queueFamilyIndexCount = 2;
		swapchainCreate.pQueueFamilyIndices = swapchainShareIndices;
	}
	else
	{
		swapchainCreate.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkSwapchainKHR vkSwapchain;
	VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCreate, NO_ALLOCATOR, &vkSwapchain));
	mist_Print("Created a swapchain!");

	return vkSwapchain;
}

void DestroyVkSwapchain(
	VkDevice device, 
	VkSwapchainKHR swapchain)
{
	vkDestroySwapchainKHR(device, swapchain, NO_ALLOCATOR);
}

VkRenderPass CreateVkRenderPass(
	VkDevice device, 
	VkSurfaceFormatKHR surfaceFormat)
{
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

	VkAttachmentReference colorReference =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorReference
	};

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
	VK_CHECK(vkCreateRenderPass(device, &createRenderPass, NO_ALLOCATOR, &vkRenderPass));
	mist_Print("Created render pass!");

	return vkRenderPass;
}

void DestroyVkRenderPass(VkDevice device, VkRenderPass renderPass)
{
	vkDestroyRenderPass(device, renderPass, NO_ALLOCATOR);
}

void CreateVkFramebuffers(
	VkDevice device, 
	VkSwapchainKHR swapchain, 
	VkSurfaceFormatKHR surfaceFormat, 
	VkRenderPass renderPass, 
	VkExtent2D surfaceExtents, 
	VkFramebuffer* framebuffers,
	VkImageView* swapchainImages,
	uint32_t framebufferCount)
{
	mist_Print("Retrieving swapchain images...");
#define vkConfig_MaxImageCount 10
	uint32_t imageCount = 0;
	VkImage swapchainPhysicalImages[vkConfig_MaxImageCount];

	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL));
	if (imageCount > vkConfig_MaxImageCount)
	{
		mist_Print("Warning: Too many images for the swapchain");
	}

	imageCount = min(vkConfig_MaxImageCount, imageCount);

	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainPhysicalImages));

	if (framebufferCount > imageCount)
	{
		mist_Print("Error: Not enough images retrieved from the swapchain!");
		assert(false);
		return;
	}

	for (uint32_t i = 0; i < framebufferCount; i++)
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

		VK_CHECK(vkCreateImageView(device, &imageViewCreate, NO_ALLOCATOR, &swapchainImages[i]));
	}
	mist_Print("Retrieved swapchain images!");

	mist_Print("Creating framebuffers...");
	VkImageView attachments[1];

	VkFramebufferCreateInfo framebufferCreate =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.attachmentCount = ARRAYSIZE(attachments),
		.width = surfaceExtents.width,
		.height = surfaceExtents.height,
		.layers = 1,
		.renderPass = renderPass,
		.pAttachments = attachments
	};

	for (uint32_t i = 0; i < framebufferCount; i++)
	{
		attachments[0] = swapchainImages[i];
		VK_CHECK(vkCreateFramebuffer(device, &framebufferCreate, NO_ALLOCATOR, &framebuffers[i]));
	}

	mist_Print("Created framebuffers!");
}

void DestroyVkFramebuffers(VkDevice device, VkFramebuffer* framebuffers, VkImageView* swapchainImages, uint32_t bufferCount)
{
	for (uint32_t i = 0; i < bufferCount; i++)
	{
		vkDestroyImageView(device, swapchainImages[i], NO_ALLOCATOR);
	}

	for (uint32_t i = 0; i < bufferCount; i++)
	{
		vkDestroyFramebuffer(device, framebuffers[i], NO_ALLOCATOR);
	}
}

VkShaderModule CreateVkShader(VkDevice device, const char* shaderPath)
{
	FILE* file = fopen(shaderPath, "rb");

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	void* shaderSource = malloc(fileSize);
	fread(shaderSource, fileSize, 1, file);
	fclose(file);

	VkShaderModuleCreateInfo createShader =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fileSize,
		.pCode = (const uint32_t*)shaderSource
	};

	VkShaderModule shader;
	VK_CHECK(vkCreateShaderModule(device, &createShader, NO_ALLOCATOR, &shader));

	free(shaderSource);

	return shader;
}

void CreateVkShaders(VkDevice device)
{
	mist_Print("Initializing shader...");
	g_VkShaders[VkShader_DefaultVert] = CreateVkShader(device, "../../Assets/Shaders/SPIR-V/default.vspv");
	g_VkShaders[VkShader_DefaultFrag] = CreateVkShader(device, "../../Assets/Shaders/SPIR-V/default.fspv");
	mist_Print("Shaders intialized!");
}

void DestroyVkShaders(VkDevice device)
{
	for (uint32_t i = 0; i < VkShader_Count; i++)
	{
		vkDestroyShaderModule(device, g_VkShaders[i], NO_ALLOCATOR);
	}
}

VkDescriptorSetLayout CreateVkDescriptorSetLayout(
	VkDevice device)
{
	mist_Print("Creating descriptor layout...");
	VkDescriptorSetLayoutCreateInfo layoutCreate =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 0,
		.pBindings = NULL
	};

	VkDescriptorSetLayout descriptorSetLayout;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutCreate, NO_ALLOCATOR, &descriptorSetLayout));
	mist_Print("Created descriptor layout!");

	return descriptorSetLayout;
}

void DestroyVkDescriptorSetLayout(
	VkDevice device, 
	VkDescriptorSetLayout descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NO_ALLOCATOR);
}

VkPipelineCache CreateVkPipelineCache(
	VkDevice device)
{
	VkPipelineCacheCreateInfo pipelineCacheCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
	};

	VkPipelineCache pipelineCache;
	VK_CHECK(vkCreatePipelineCache(device, &pipelineCacheCreate, NO_ALLOCATOR, &pipelineCache));

	return pipelineCache;
}

void DestroyVkPipelineCache(
	VkDevice device,
	VkPipelineCache cache)
{
	vkDestroyPipelineCache(device, cache, NO_ALLOCATOR);
}

VkPipelineLayout CreateVkPipelineLayout(
	VkDevice device,
	VkDescriptorSetLayout descriptorSetLayout)
{
	mist_Print("Creating pipeline layout...");
	VkPipelineLayoutCreateInfo pipelineLayoutCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout
	};

	VkPipelineLayout pipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreate, NO_ALLOCATOR, &pipelineLayout));
	mist_Print("Pipeline layout created!");

	return pipelineLayout;
}

void DestroyVkPipelineLayout(
	VkDevice device,
	VkDescriptorSetLayout layout)
{
	vkDestroyPipelineLayout(device, layout, NO_ALLOCATOR);
}

VkPipeline CreateVkPipeline(
	VkDevice device,  
	VkRenderPass renderPass, 
	VkShaderModule vertShader, 
	VkShaderModule fragShader,
	VkPipelineCache pipelineCache,
	VkPipelineLayout pipelineLayout,
	uint32_t surfaceWidth, 
	uint32_t surfaceHeight)
{
	mist_Print("Initializing pipeline...");

	mist_Print("Vertex Input...");

	// Vertex Input
	VkVertexInputBindingDescription vertexBinding =
	{
		.binding = vkConfig_VertexBinding,
		.stride = sizeof(mist_VkVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputBindingDescription instanceBinding =
	{
		.binding = vkConfig_InstanceBinding,
		.stride = sizeof(mist_VkInstance),
		.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
	};

	VkVertexInputBindingDescription inputBindings[] =
	{
		vertexBinding,
		instanceBinding
	};


	VkVertexInputAttributeDescription vertexPositionAttribute =
	{
		.binding = vkConfig_VertexBinding,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.location = 0,
		.offset = offsetof(mist_VkVertex, position)
	};

	VkVertexInputAttributeDescription instancePositionAttribute =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.location = 1,
		.offset = offsetof(mist_VkInstance, position)
	};

	VkVertexInputAttributeDescription instanceScaleAttribute =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.location = 2,
		.offset = offsetof(mist_VkInstance, scale)
	};

	VkVertexInputAttributeDescription inputAttributes[] =
	{
		vertexPositionAttribute,
		instancePositionAttribute,
		instanceScaleAttribute
	};

	VkPipelineVertexInputStateCreateInfo vertexInputCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = ARRAYSIZE(inputBindings),
		.pVertexBindingDescriptions = inputBindings,
		.vertexAttributeDescriptionCount = ARRAYSIZE(inputAttributes),
		.pVertexAttributeDescriptions = inputAttributes
	};

	mist_Print("Input assembly...");

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreate =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
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
		{ .x = 0,.y = 0 },
		{ .width = surfaceWidth ,.height = surfaceHeight }
	};

	VkViewport viewport =
	{
		.x = 0,
		.y = 0,
		.width = surfaceWidth,
		.height = surfaceHeight,
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
		.renderPass = renderPass,
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

	VkPipeline pipeline;
	VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &graphicsPipelineCreate, NO_ALLOCATOR, &pipeline));

	mist_Print("Pipeline initialized!");
	return pipeline;
}

void DestroyVkPipeline(VkDevice device, VkPipeline pipeline)
{
	vkDestroyPipeline(device, pipeline, NO_ALLOCATOR);
}

uint32_t FindMemoryIndex(PhysicalDevice* physicalDevice, VkMemoryPropertyFlags requiredFlags, VkMemoryPropertyFlags preferedFlags)
{
	uint32_t preferedMemoryIndex = UINT32_MAX;
	VkMemoryType* types = physicalDevice->memoryProperties.memoryTypes;

	for (uint32_t i = 0; i < physicalDevice->memoryProperties.memoryTypeCount; ++i)
	{
		if ((types[i].propertyFlags & (requiredFlags | preferedFlags)) == (requiredFlags | preferedFlags))
		{
			return types[i].heapIndex;
		}
	}

	if (preferedMemoryIndex == UINT32_MAX)
	{
		for (uint32_t i = 0; i < physicalDevice->memoryProperties.memoryTypeCount; ++i)
		{
			if ((types[i].propertyFlags & requiredFlags) == requiredFlags)
			{
				return i;
			}
		}
	}

	return UINT32_MAX;
}

void CreateVkDescriptorPools(
	VkDevice device, 
	VkDescriptorPool* descriptorPools, 
	uint32_t poolCount)
{
	#define vkConfig_MaxUniformBufferCount 1000
	#define vkConfig_MaxImageSamples 1000
	VkDescriptorPoolSize descriptorPoolSizes[2] =
	{
		{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,.descriptorCount = vkConfig_MaxUniformBufferCount },
		{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vkConfig_MaxImageSamples }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreate =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = vkConfig_MaxDescriptorSets,
		.poolSizeCount = ARRAYSIZE(descriptorPoolSizes),
		.pPoolSizes = descriptorPoolSizes
	};

	for (uint32_t i = 0; i < poolCount; i++)
	{
		VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreate, NO_ALLOCATOR, &descriptorPools[i]));
	}

	mist_Print("Created descriptor pool!");
}

void DestroyVkDescriptorPools(VkDevice device, VkDescriptorPool* descriptorPools, uint32_t poolCount)
{
	for (uint32_t i = 0; i < poolCount; i++)
	{
		VK_CHECK(vkResetDescriptorPool(device, descriptorPools[i], 0));
		vkDestroyDescriptorPool(device, descriptorPools[i], NO_ALLOCATOR);
	}
}

void InitVkAllocator(
	PhysicalDevice* physicalDevice, 
	VkDevice device)
{
	VkMemoryPropertyFlags requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VkMemoryPropertyFlags preferedFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	mist_Print("Initializing allocator...");
	mist_InitVkAllocator(&g_VkAllocator, device, 1024 * 1024, FindMemoryIndex(physicalDevice, requiredFlags, preferedFlags), mist_AllocatorHostVisible);
	mist_Print("Initialized allocator!");
}

void KillVkAllocator(
	VkDevice device)
{
	mist_Print("Cleaning up allocator!");
	mist_CleanupVkAllocator(&g_VkAllocator, device);
	mist_Print("Cleaned up allocator!");
}

void RecordCommandBuffers(uint32_t surfaceWidth, uint32_t surfaceHeight)
{
	static VkDescriptorSet s_DescriptorSets[vkConfig_BufferCount];

	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VkCommandBufferBeginInfo beginBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};
		VK_CHECK(vkBeginCommandBuffer(g_GraphicsCommandBuffers[i], &beginBufferInfo));

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
		vkCmdPipelineBarrier(g_GraphicsCommandBuffers[i],
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 1, &barrier, 0, NULL, 0, NULL);

		VkClearColorValue clearColor = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } };
		VkClearValue clearValues[1] =
		{
			{ .color = clearColor }
		};

		VkRenderPassBeginInfo renderPassBeginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = g_VkRenderPass,
			.framebuffer = g_VkFramebuffers[i],
			.renderArea = { .extent = g_VkSurfaceExtents },
			.clearValueCount = ARRAYSIZE(clearValues),
			.pClearValues = clearValues
		};
		vkCmdBeginRenderPass(g_GraphicsCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkClearAttachment clearAttachment =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.colorAttachment = 0,
			.clearValue = { .color = clearColor }
		};

		VkClearRect clearRect =
		{
			.baseArrayLayer = 0,
			.layerCount = 1,
			.rect = { .offset = { 0, 0 },.extent = { surfaceWidth, surfaceHeight } }
		};
		vkCmdClearAttachments(g_GraphicsCommandBuffers[i], 1, &clearAttachment, 1, &clearRect);

		// Setup the pipeline
		VkDescriptorSetAllocateInfo descriptorSetAlloc =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = g_VkDescriptorPools[i],
			.descriptorSetCount = 1,
			.pSetLayouts = &g_VkDescriptorSetLayout
		};
		VK_CHECK(vkAllocateDescriptorSets(g_VkDevice, &descriptorSetAlloc, &s_DescriptorSets[i]));
		VkDescriptorSet descriptorSet = s_DescriptorSets[i];
		vkCmdBindDescriptorSets(g_GraphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, g_VkPipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		vkCmdBindPipeline(g_GraphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, g_VkPipeline);

		// Draw stuff
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(g_GraphicsCommandBuffers[i], vkConfig_VertexBinding, 1, &g_VkMeshes[VkMesh_Rect].buffer, &offset);
		vkCmdBindVertexBuffers(g_GraphicsCommandBuffers[i], vkConfig_InstanceBinding, 1, &g_VkInstances[VkMesh_Rect].buffer, &offset);
		vkCmdDrawIndirect(g_GraphicsCommandBuffers[i], g_VkIndirectDrawBuffer.buffer, VkMesh_Rect * sizeof(VkDrawIndirectCommand), 1, sizeof(VkDrawIndirectCommand));

		// Finish the frame
		vkCmdEndRenderPass(g_GraphicsCommandBuffers[i]);
		vkEndCommandBuffer(g_GraphicsCommandBuffers[i]);
	}
}

mist_VkBuffer CreateVkBuffer(VkDevice device, mist_VkAllocator* allocator, void* data, VkDeviceSize size, VkBufferUsageFlags usage)
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

	if (data != NULL)
	{
		memcpy(mappedMemory, data, size);
	}
	return (mist_VkBuffer) { .alloc = allocation, .mappedMem = mappedMemory, .buffer = buffer };
}

void DestroyVkBuffer(VkDevice device, mist_VkAllocator* allocator, mist_VkBuffer buffer)
{
	vkDestroyBuffer(device, buffer.buffer, NO_ALLOCATOR);
	mist_VkFree(allocator, buffer.alloc);
}

void CreateVkMeshes(VkDevice device, mist_VkAllocator* allocator)
{
	// Rect
	mist_VkVertex vertices[] =
	{
		{ .position = { -1.0f, -1.0f } },
		{ .position = { -1.0f,  1.0f } },
		{ .position = {  1.0f, -1.0f } },
		{ .position = {  1.0f,  1.0f } }
	};

	VkDrawIndirectCommand indirectDraws[VkMesh_Count] = 
	{
		[VkMesh_Rect] = { .vertexCount = 4 }
	};

	g_VkIndirectDrawBuffer = CreateVkBuffer(device, allocator, indirectDraws, sizeof(VkDrawIndirectCommand) * VkMesh_Count, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
	g_VkMeshes[VkMesh_Rect] = CreateVkBuffer(device, allocator, vertices, sizeof(mist_VkVertex) * ARRAYSIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	g_VkInstances[VkMesh_Rect] = CreateVkBuffer(device, allocator, NULL, sizeof(mist_VkInstance) * vkConfig_MaxInstanceCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void DestroyVkMeshes(VkDevice device, mist_VkAllocator* allocator)
{
	DestroyVkBuffer(device, allocator, g_VkMeshes[VkMesh_Rect]);
	DestroyVkBuffer(device, allocator, g_VkInstances[VkMesh_Rect]);
	DestroyVkBuffer(device, allocator, g_VkIndirectDrawBuffer);
}

void VkRenderer_Init(
	HINSTANCE appInstance, 
	HWND window, 
	uint32_t surfaceWidth, 
	uint32_t surfaceHeight)
{
	g_VkInstance = CreateVkInstance();
	g_VkSurface = CreateVkSurface(appInstance, window, g_VkInstance);
	EnumeratePhysicalDevices(g_VkInstance, g_VkSurface);

	PhysicalDevice* selectedDevice;
	SelectBestPhysicalDevice(g_VkSurface, &selectedDevice, &g_VkGraphicsQueueIdx, &g_VkPresentQueueIdx);
	CreateVkLogicDevice(selectedDevice->device, g_VkGraphicsQueueIdx, g_VkPresentQueueIdx, &g_VkDevice, &g_VkGraphicsQueue, &g_VkPresentQueue);

	CreateVkSemaphores(g_VkDevice, g_AcquireSemaphores, vkConfig_BufferCount);
	CreateVkSemaphores(g_VkDevice, g_FrameCompleteSemaphores, vkConfig_BufferCount);

	CreateVkCommandBuffers(g_VkDevice, g_VkGraphicsQueueIdx, &g_GraphicsCommandPool, g_GraphicsCommandBuffers, vkConfig_BufferCount);
	CreateVkFences(g_VkDevice, g_GraphicsFences, vkConfig_BufferCount);

	VkSurfaceFormatKHR surfaceFormat = SelectVkSurfaceFormat(selectedDevice);
	g_VkSurfaceExtents = CreateSurfaceExtents(selectedDevice, surfaceWidth, surfaceHeight);
	g_VkSwapchain = CreateVkSwapchain(selectedDevice, g_VkDevice, g_VkSurface, surfaceFormat, g_VkSurfaceExtents, g_VkGraphicsQueueIdx, g_VkPresentQueueIdx, vkConfig_BufferCount);
	g_VkRenderPass = CreateVkRenderPass(g_VkDevice, surfaceFormat);

	CreateVkFramebuffers(g_VkDevice, g_VkSwapchain, surfaceFormat, g_VkRenderPass, g_VkSurfaceExtents, g_VkFramebuffers, g_VkSwapchainImages, vkConfig_BufferCount);
	CreateVkShaders(g_VkDevice);


	g_VkDescriptorSetLayout = CreateVkDescriptorSetLayout(g_VkDevice);
	g_VkPipelineLayout = CreateVkPipelineLayout(g_VkDevice, g_VkDescriptorSetLayout);
	g_VkPipelineCache = CreateVkPipelineCache(g_VkDevice);
	g_VkPipeline = CreateVkPipeline(g_VkDevice, g_VkRenderPass, g_VkShaders[VkShader_DefaultVert], g_VkShaders[VkShader_DefaultFrag], g_VkPipelineCache, g_VkPipelineLayout, surfaceWidth, surfaceHeight);

	CreateVkDescriptorPools(g_VkDevice, g_VkDescriptorPools, vkConfig_BufferCount);

	InitVkAllocator(selectedDevice, g_VkDevice);
	CreateVkMeshes(g_VkDevice, &g_VkAllocator);

	RecordCommandBuffers(surfaceWidth, surfaceHeight);
}

void VkRenderer_Kill()
{
	DestroyVkMeshes(g_VkDevice, &g_VkAllocator);
	KillVkAllocator(g_VkDevice);

	DestroyVkDescriptorPools(g_VkDevice, g_VkDescriptorPools, vkConfig_BufferCount);
	DestroyVkPipeline(g_VkDevice, g_VkPipeline);
	DestroyVkPipelineCache(g_VkDevice, g_VkPipelineCache);
	DestroyVkPipelineLayout(g_VkDevice, g_VkPipelineLayout);
	DestroyVkDescriptorSetLayout(g_VkDevice, g_VkDescriptorSetLayout);
	DestroyVkShaders(g_VkDevice);
	DestroyVkFences(g_VkDevice, g_GraphicsFences, vkConfig_BufferCount);
	DestroyVkCommandBuffers(g_VkDevice, g_GraphicsCommandPool);
	DestroyVkSemaphores(g_VkDevice, g_FrameCompleteSemaphores, vkConfig_BufferCount);
	DestroyVkSemaphores(g_VkDevice, g_AcquireSemaphores, vkConfig_BufferCount);
	DestroyVkFramebuffers(g_VkDevice, g_VkFramebuffers, g_VkSwapchainImages, vkConfig_BufferCount);
	DestroyVkRenderPass(g_VkDevice, g_VkRenderPass);
	DestroyVkSwapchain(g_VkDevice, g_VkSwapchain);
	DestroyVkLogicDevice(g_VkDevice);
	DestroyVkSurface(g_VkInstance, g_VkSurface);
	DestroyVkInstance(g_VkInstance);
}

void VkRenderer_Draw()
{
	static uint8_t s_CurrentFrame = 0;

	// Start the frame
	uint32_t imageIndex = 0;
	VK_CHECK(vkAcquireNextImageKHR(g_VkDevice, g_VkSwapchain, UINT64_MAX, g_AcquireSemaphores[s_CurrentFrame], VK_NULL_HANDLE, &imageIndex));

	VkSemaphore* acquire = &g_AcquireSemaphores[s_CurrentFrame];
	VkSemaphore* finished = &g_FrameCompleteSemaphores[s_CurrentFrame];

	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &g_GraphicsCommandBuffers[s_CurrentFrame],
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = acquire,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = finished,
		.pWaitDstStageMask = &dstStageMask
	};

	VK_CHECK(vkQueueSubmit(g_VkGraphicsQueue, 1, &submitInfo, g_GraphicsFences[s_CurrentFrame]));

	// Present the frame
	VK_CHECK(vkWaitForFences(g_VkDevice, 1, &g_GraphicsFences[s_CurrentFrame], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(g_VkDevice, 1, &g_GraphicsFences[s_CurrentFrame]));

	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = finished,
		.swapchainCount = 1,
		.pSwapchains = &g_VkSwapchain,
		.pImageIndices = &imageIndex
	};
	VK_CHECK(vkQueuePresentKHR(g_VkPresentQueue, &presentInfo));

	s_CurrentFrame = (s_CurrentFrame + 1) % vkConfig_BufferCount;
}

void VkRenderer_ClearInstances()
{
	for (uint32_t i = 0; i < VkMesh_Count; i++)
	{
		VkDrawIndirectCommand* indirectCommand = (VkDrawIndirectCommand*)g_VkIndirectDrawBuffer.mappedMem + i;
		indirectCommand->instanceCount = 0;
	}
}

void VkRenderer_AddInstance(mist_VkMesh mesh, mist_Vec2 position, mist_Vec2 scale)
{
	VkDrawIndirectCommand* indirectCommand = (VkDrawIndirectCommand*)g_VkIndirectDrawBuffer.mappedMem + mesh;

	mist_VkInstance* instance = (mist_VkInstance*)g_VkInstances[mesh].mappedMem + indirectCommand->instanceCount;
	instance->position = position;
	instance->scale = scale;

	assert(indirectCommand->instanceCount + 1 < vkConfig_MaxInstanceCount);
	indirectCommand->instanceCount++;
}