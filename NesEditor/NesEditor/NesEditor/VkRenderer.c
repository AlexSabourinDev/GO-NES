#include "VkRenderer.h"

#include <stdio.h>

// Vulkan!
#define VK_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "MistVk.h"
#include "VkAllocator.h"

#include "FontFormat.h"

// stb
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

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

typedef struct mist_PhysicalDevice
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
} mist_PhysicalDevice;

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

mist_PhysicalDevice g_PhysicalDevices[vkConfig_MaxPhysicalDevices];
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
VkDescriptorSet g_VkDescriptorSets[vkConfig_BufferCount];

mist_VkAllocator g_VkAllocator;
mist_PhysicalDevice* g_VkSelectedDevice;

typedef struct mist_VkVertex
{
	mist_Vec2 position;
} mist_VkVertex;

typedef struct mist_VkInstance
{
	mist_Vec2 position;
	mist_Vec2 scale;
	mist_Color color;
	unsigned int stringSize;
	unsigned int stringBuffer[4];

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

typedef struct mist_VkImage
{
	mist_VkAlloc alloc;
	VkImage image;
	VkImageView imageView;
	VkSampler sampler;
} mist_VkImage;

typedef struct mist_VkUniformBuffer
{
	mist_Vec2 surfaceDimensions;
	unsigned int fontTileWidth;
	unsigned int fontTileHeight;
	unsigned int fontWidth;
	unsigned int fontHeight;
} mist_VkUniformBuffer;

mist_VkBuffer g_VkMeshes[VkMesh_Count];
mist_VkBuffer g_VkInstances[VkMesh_Count];

mist_VkBuffer g_VkIndirectDrawBuffer;

mist_VkBuffer g_VkDescriptorBuffers[vkConfig_BufferCount];

mist_VkImage g_VkFontImage;

VkInstance CreateVkInstance(void)
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
	mist_PhysicalDevice** selectedGPU,
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
			mist_PhysicalDevice* physicalDevice = &g_PhysicalDevices[i];

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

VkCommandBuffer BeginOneTimeVkCommandBuffer(
	VkDevice device,
	VkCommandPool pool)
{
	VkCommandBuffer oneTimeBuffer;

	VkCommandBufferAllocateInfo commandBufferAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
		.commandPool = pool
	};

	VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &oneTimeBuffer));

	VkCommandBufferBeginInfo beginBufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	VK_CHECK(vkBeginCommandBuffer(oneTimeBuffer, &beginBufferInfo));

	return oneTimeBuffer;
}

void EndOneTimeVkCommandBuffer(
	VkDevice device,
	VkCommandBuffer buffer,
	VkQueue submissionQueue,
	VkCommandPool pool)
{
	VK_CHECK(vkEndCommandBuffer(buffer));

	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &buffer
	};
	VK_CHECK(vkQueueSubmit(submissionQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(submissionQueue));

	vkFreeCommandBuffers(device, pool, 1, &buffer);
}

void TransitionVkImageLayout(
	VkImage image,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkCommandBuffer buffer)
{
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	VkAccessFlagBits sourceAccessMask;
	VkAccessFlagBits dstAccessMask;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		sourceAccessMask = 0;
		dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		sourceAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else 
	{
		assert(false);
		return;
	}

	VkImageMemoryBarrier imageBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,.levelCount = 1, .baseMipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
		.srcAccessMask = sourceAccessMask,
		.dstAccessMask = dstAccessMask
	};

	vkCmdPipelineBarrier(buffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &imageBarrier);
}

void CopyBufferToImage(
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height,
	VkCommandBuffer commandBuffer)
{
	VkBufferImageCopy copyRegion = 
	{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
		.imageOffset = {.x = 0, .y = 0, .z = 0},
		.imageExtent = {.width = width, .height = height, .depth = 1}
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
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
	mist_PhysicalDevice* physicalDevice)
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
		surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
		surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	else
	{
		surfaceFormat = physicalDevice->surfaceFormats[0];
		for (uint32_t i = 0; i < physicalDevice->surfaceFormatCount; i++)
		{
			if (VK_FORMAT_R8G8B8A8_UNORM == physicalDevice->surfaceFormats[i].format && VK_COLORSPACE_SRGB_NONLINEAR_KHR == physicalDevice->surfaceFormats[i].colorSpace)
			{
				surfaceFormat = physicalDevice->surfaceFormats[i];
				break;
			}
		}
	}
	mist_Print("Selected surface format!");

	return surfaceFormat;
}

VkExtent2D CreateSurfaceExtents(
	mist_PhysicalDevice* device,
	uint32_t width,
	uint32_t height)
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
	mist_PhysicalDevice* physicalDevice,
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
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
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

	VkSubpassDependency dependency = 
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	VkRenderPassCreateInfo createRenderPass =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = ARRAYSIZE(renderPassAttachments),
		.subpassCount = 1,
		.dependencyCount = 1,
		.pDependencies = &dependency,
		.pAttachments = renderPassAttachments,
		.pSubpasses = &subpass
	};

	VkRenderPass vkRenderPass;
	VK_CHECK(vkCreateRenderPass(device, &createRenderPass, NO_ALLOCATOR, &vkRenderPass));
	mist_Print("Created render pass!");

	return vkRenderPass;
}

void DestroyVkRenderPass(
	VkDevice device,
	VkRenderPass renderPass)
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

void DestroyVkFramebuffers(
	VkDevice device,
	VkFramebuffer* framebuffers,
	VkImageView* swapchainImages,
	uint32_t bufferCount)
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

VkShaderModule CreateVkShader(
	VkDevice device, 
	const char* shaderPath)
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

void CreateVkShaders(
	VkDevice device)
{
	mist_Print("Initializing shader...");
	g_VkShaders[VkShader_DefaultVert] = CreateVkShader(device, "../../Assets/Shaders/SPIR-V/default.vspv");
	g_VkShaders[VkShader_DefaultFrag] = CreateVkShader(device, "../../Assets/Shaders/SPIR-V/default.fspv");
	mist_Print("Shaders intialized!");
}

void DestroyVkShaders(
	VkDevice device)
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

	VkDescriptorSetLayoutBinding screenSizeBinding =
	{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1
	};

	VkDescriptorSetLayoutBinding fontImageBinding =
	{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1
	};


	VkDescriptorSetLayoutBinding layoutBindings[] = 
	{
		screenSizeBinding,
		fontImageBinding
	};
	VkDescriptorSetLayoutCreateInfo layoutCreate =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 2,
		.pBindings = layoutBindings
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
		.format = VK_FORMAT_R32G32_SFLOAT,
		.location = 0,
		.offset = offsetof(mist_VkVertex, position)
	};

	VkVertexInputAttributeDescription instancePositionAttribute =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.location = 1,
		.offset = offsetof(mist_VkInstance, position)
	};

	VkVertexInputAttributeDescription instanceScaleAttribute =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.location = 2,
		.offset = offsetof(mist_VkInstance, scale)
	};

	VkVertexInputAttributeDescription instanceColorAttribute =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.location = 3,
		.offset = offsetof(mist_VkInstance, color)
	};

	VkVertexInputAttributeDescription instanceStringSize =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32_UINT,
		.location = 4,
		.offset = offsetof(mist_VkInstance, stringSize)
	};

	VkVertexInputAttributeDescription instanceStringBuffer =
	{
		.binding = vkConfig_InstanceBinding,
		.format = VK_FORMAT_R32G32B32A32_UINT,
		.location = 5,
		.offset = offsetof(mist_VkInstance, stringBuffer)
	};

	VkVertexInputAttributeDescription inputAttributes[] =
	{
		vertexPositionAttribute,
		instancePositionAttribute,
		instanceScaleAttribute,
		instanceColorAttribute,
		instanceStringSize,
		instanceStringBuffer
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

void DestroyVkPipeline(
	VkDevice device,
	VkPipeline pipeline)
{
	vkDestroyPipeline(device, pipeline, NO_ALLOCATOR);
}

uint32_t FindMemoryIndex(
	mist_PhysicalDevice* physicalDevice,
	uint32_t typeBits,
	VkMemoryPropertyFlags requiredFlags,
	VkMemoryPropertyFlags preferedFlags)
{
	uint32_t preferedMemoryIndex = UINT32_MAX;
	VkMemoryType* types = physicalDevice->memoryProperties.memoryTypes;

	for (uint32_t i = 0; i < physicalDevice->memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeBits & (1 << i)) && (types[i].propertyFlags & (requiredFlags | preferedFlags)) == (requiredFlags | preferedFlags))
		{
			return i;
		}
	}

	if (preferedMemoryIndex == UINT32_MAX)
	{
		for (uint32_t i = 0; i < physicalDevice->memoryProperties.memoryTypeCount; ++i)
		{
			if ((typeBits & (1 << i)) && (types[i].propertyFlags & requiredFlags) == requiredFlags)
			{
				return i;
			}
		}
	}

	return UINT32_MAX;
}

void InitVkAllocators(
	VkDevice device)
{
	mist_Print("Initializing allocator...");
	mist_InitVkAllocator(&g_VkAllocator, device);
	mist_Print("Initialized allocator!");
}

void KillVkAllocators(void)
{
	mist_Print("Cleaning up allocator!");
	mist_CleanupVkAllocator(&g_VkAllocator);
	mist_Print("Cleaned up allocator!");
}

mist_VkBuffer CreateVkBuffer(
	VkDevice device,
	mist_PhysicalDevice* physicalDevice,
	mist_VkAllocator* allocator,
	void* data, VkDeviceSize size,
	VkBufferUsageFlags usage)
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

	mist_VkAlloc allocation = mist_VkAllocate(allocator, 
		FindMemoryIndex(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), 
		memoryRequirements.size, memoryRequirements.alignment, mist_AllocatorHostVisible);
	VK_CHECK(vkBindBufferMemory(device, buffer, allocation.memory, allocation.offset));

	void* mappedMemory = mist_VkMapMemory(allocation);

	if (data != NULL)
	{
		memcpy(mappedMemory, data, size);
	}
	return (mist_VkBuffer) { .alloc = allocation, .mappedMem = mappedMemory, .buffer = buffer };
}

void DestroyVkBuffer(
	VkDevice device,
	mist_VkBuffer buffer)
{
	vkDestroyBuffer(device, buffer.buffer, NO_ALLOCATOR);
	mist_VkFree(buffer.alloc);
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

void DestroyVkDescriptorPools(
	VkDevice device,
	VkDescriptorPool* descriptorPools,
	uint32_t poolCount)
{
	for (uint32_t i = 0; i < poolCount; i++)
	{
		VK_CHECK(vkResetDescriptorPool(device, descriptorPools[i], 0));
		vkDestroyDescriptorPool(device, descriptorPools[i], NO_ALLOCATOR);
	}
}

void CreateVkDescriptorSet(
	VkDevice device,
	mist_PhysicalDevice* physicalDevice,
	mist_VkAllocator* allocator,
	uint32_t surfaceWidth,
	uint32_t surfaceHeight,
	mist_VkImage* fontImage,
	VkDescriptorSetLayout descriptorSetLayout, 
	mist_VkBuffer* descriptorBuffers,
	VkDescriptorPool* descriptorPools,
	VkDescriptorSet* descriptorSets,
	uint32_t descriptorSetCount)
{
	mist_VkUniformBuffer uniformBuffer =
	{
		.surfaceDimensions = {.x = (float)surfaceWidth,.y = (float)surfaceHeight },
		.fontTileWidth = mist_FontTileWidth,
		.fontTileHeight = mist_FontTileHeight,
		.fontWidth = mist_FontWidth,
		.fontHeight = mist_FontHeight
	};

	for (uint32_t i = 0; i < descriptorSetCount; i++)
	{
		descriptorBuffers[i] = CreateVkBuffer(device, physicalDevice, allocator, &uniformBuffer, sizeof(mist_VkUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

		VkDescriptorSetAllocateInfo descriptorSetAlloc =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPools[i],
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptorSetLayout
		};
		VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAlloc, &descriptorSets[i]));

		VkDescriptorBufferInfo bufferInfo =
		{
			.buffer = descriptorBuffers[i].buffer,
			.offset = 0,
			.range = sizeof(mist_VkUniformBuffer)
		};

		// Binding 0 : Vertex shader uniform buffer
		VkWriteDescriptorSet writeVertexUniformDescriptorSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.dstBinding = 0,
			.descriptorCount = 1,
			.pBufferInfo = &bufferInfo
		};

		VkDescriptorImageInfo imageInfo =
		{
			.imageView = fontImage->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.sampler = fontImage->sampler
		};

		VkWriteDescriptorSet writeImageDescriptorSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.dstBinding = 1,
			.descriptorCount = 1,
			.pImageInfo = &imageInfo
		};

		VkWriteDescriptorSet writeDescriptorSets[] =
		{
			writeVertexUniformDescriptorSet,
			writeImageDescriptorSet
		};

		vkUpdateDescriptorSets(device, ARRAYSIZE(writeDescriptorSets), writeDescriptorSets, 0, NULL);
	}
}

void DestroyVkDescriptorSets(
	VkDevice device,
	mist_VkBuffer* buffers, 
	uint32_t bufferCount)
{
	for (uint32_t i = 0; i < bufferCount; i++)
	{
		DestroyVkBuffer(device, buffers[i]);
	}
}

void UpdateVkDescriptorSets(
	VkDevice device,
	uint32_t surfaceWidth,
	uint32_t surfaceHeight,
	mist_VkImage* fontImage,
	mist_VkBuffer* descriptorBuffers,
	VkDescriptorSet* descriptorSets,
	uint32_t descriptorSetCount)
{
	for (uint32_t i = 0; i < descriptorSetCount; i++)
	{
		mist_VkUniformBuffer* uniformBuffer = (mist_VkUniformBuffer*)descriptorBuffers[i].mappedMem;
		uniformBuffer->surfaceDimensions = (mist_Vec2){ .x = (float)surfaceWidth,.y = (float)surfaceHeight };
		memcpy((uint8_t*)descriptorBuffers[i].mappedMem + offsetof(mist_VkUniformBuffer, surfaceDimensions), uniformBuffer, sizeof(mist_Vec2));

		VkDescriptorBufferInfo bufferInfo =
		{
			.buffer = descriptorBuffers[i].buffer,
			.offset = 0,
			.range = sizeof(float) * 2
		};

		// Binding 0 : Vertex shader uniform buffer
		VkWriteDescriptorSet writeVertexUniformDescriptorSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.dstBinding = 0,
			.descriptorCount = 1,
			.pBufferInfo = &bufferInfo
		};

		VkDescriptorImageInfo imageInfo = 
		{
			.imageView = fontImage->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.sampler = fontImage->sampler
		};

		VkWriteDescriptorSet writeImageDescriptorSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.dstBinding = 1,
			.descriptorCount = 1,
			.pImageInfo = &imageInfo
		};

		VkWriteDescriptorSet writeDescriptorSets[] =
		{
			writeVertexUniformDescriptorSet,
			writeImageDescriptorSet
		};

		vkUpdateDescriptorSets(device, ARRAYSIZE(writeDescriptorSets), writeDescriptorSets, 0, NULL);
	}
}

void RecordCommandBuffers(void)
{
	for (uint32_t i = 0; i < vkConfig_BufferCount; i++)
	{
		VK_CHECK(vkResetCommandBuffer(g_GraphicsCommandBuffers[i], 0));

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

		// Setup the pipeline
		vkCmdBindDescriptorSets(g_GraphicsCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, g_VkPipelineLayout, 0, 1, &g_VkDescriptorSets[i], 0, NULL);

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

void RecreateVkSwapchain(
	uint32_t surfaceWidth,
	uint32_t surfaceHeight)
{
	vkDeviceWaitIdle(g_VkDevice);

	DestroyVkPipeline(g_VkDevice, g_VkPipeline);

	DestroyVkFramebuffers(g_VkDevice, g_VkFramebuffers, g_VkSwapchainImages, vkConfig_BufferCount);

	VK_CHECK(vkResetCommandPool(g_VkDevice, g_GraphicsCommandPool, 0));
	DestroyVkSwapchain(g_VkDevice, g_VkSwapchain);

	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_VkSelectedDevice->device, g_VkSurface, &g_VkSelectedDevice->surfaceCapabilities));
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(g_VkSelectedDevice->device, g_VkSurface, &g_VkSelectedDevice->surfaceFormatCount, g_VkSelectedDevice->surfaceFormats));
	VkSurfaceFormatKHR surfaceFormat = SelectVkSurfaceFormat(g_VkSelectedDevice);
	g_VkSurfaceExtents = CreateSurfaceExtents(g_VkSelectedDevice, surfaceWidth, surfaceHeight);
	g_VkSwapchain = CreateVkSwapchain(g_VkSelectedDevice, g_VkDevice, g_VkSurface, surfaceFormat, g_VkSurfaceExtents, g_VkGraphicsQueueIdx, g_VkPresentQueueIdx, vkConfig_BufferCount);

	CreateVkFramebuffers(g_VkDevice, g_VkSwapchain, surfaceFormat, g_VkRenderPass, g_VkSurfaceExtents, g_VkFramebuffers, g_VkSwapchainImages, vkConfig_BufferCount);
	UpdateVkDescriptorSets(g_VkDevice, surfaceWidth, surfaceHeight, &g_VkFontImage, g_VkDescriptorBuffers, g_VkDescriptorSets, vkConfig_BufferCount);

	g_VkPipeline = CreateVkPipeline(g_VkDevice, g_VkRenderPass, g_VkShaders[VkShader_DefaultVert], g_VkShaders[VkShader_DefaultFrag], g_VkPipelineCache, g_VkPipelineLayout, surfaceWidth, surfaceHeight);

	RecordCommandBuffers();
}

void CreateVkMeshes(
	VkDevice device, 
	mist_PhysicalDevice* physicalDevice,
	mist_VkAllocator* allocator)
{
	// Rect
	mist_VkVertex vertices[] =
	{
		{ .position = { -0.5f,  0.5f } },
		{ .position = { -0.5f, -0.5f } },
		{ .position = {  0.5f,  0.5f } },
		{ .position = {  0.5f, -0.5f } }
	};

	VkDrawIndirectCommand indirectDraws[VkMesh_Count] = 
	{
		[VkMesh_Rect] = { .vertexCount = 4 }
	};

	g_VkIndirectDrawBuffer = CreateVkBuffer(device, physicalDevice, allocator, indirectDraws, sizeof(VkDrawIndirectCommand) * VkMesh_Count, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
	g_VkMeshes[VkMesh_Rect] = CreateVkBuffer(device, physicalDevice, allocator, vertices, sizeof(mist_VkVertex) * ARRAYSIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	g_VkInstances[VkMesh_Rect] = CreateVkBuffer(device, physicalDevice, allocator, NULL, sizeof(mist_VkInstance) * vkConfig_MaxInstanceCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void DestroyVkMeshes(
	VkDevice device)
{
	DestroyVkBuffer(device, g_VkMeshes[VkMesh_Rect]);
	DestroyVkBuffer(device, g_VkInstances[VkMesh_Rect]);
	DestroyVkBuffer(device, g_VkIndirectDrawBuffer);
}

mist_VkImage CreateVkImage(
	VkDevice device,
	mist_PhysicalDevice* physicalDevice,
	mist_VkAllocator* allocator,
	void* memory,
	uint32_t x,
	uint32_t y,
	uint32_t n,
	VkCommandPool commandPool,
	VkQueue imageQueue)
{
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(g_VkSelectedDevice->device, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);

	VkImageCreateInfo imageCreate =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.extent = { .width = x,.height = y,.depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT ,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkImage createdImage;
	VK_CHECK(vkCreateImage(device, &imageCreate, NO_ALLOCATOR, &createdImage));

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(device, createdImage, &memReq);

	mist_VkAlloc imageAlloc = mist_VkAllocate(allocator,
		FindMemoryIndex(physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0),
		memReq.size, memReq.alignment, mist_AllocatorNone);
	VK_CHECK(vkBindImageMemory(device, createdImage, imageAlloc.memory, imageAlloc.offset));

	mist_VkBuffer imageBuffer = CreateVkBuffer(device, physicalDevice, allocator, memory, x*y*n, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	VkCommandBuffer imageCopyCommandBuffer = BeginOneTimeVkCommandBuffer(device, commandPool);
	TransitionVkImageLayout(createdImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageCopyCommandBuffer);
	CopyBufferToImage(imageBuffer.buffer, createdImage, x, y, imageCopyCommandBuffer);
	TransitionVkImageLayout(createdImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageCopyCommandBuffer);
	EndOneTimeVkCommandBuffer(device, imageCopyCommandBuffer, imageQueue, commandPool);
	DestroyVkBuffer(device, imageBuffer);

	VkImageViewCreateInfo  imageCreateViewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = createdImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM,

		.components = { .r = VK_COMPONENT_SWIZZLE_R,.g = VK_COMPONENT_SWIZZLE_G,.b = VK_COMPONENT_SWIZZLE_B,.a = VK_COMPONENT_SWIZZLE_A },
		.subresourceRange =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
			.baseMipLevel = 0
		}
	};

	VkImageView createdImageView;
	VK_CHECK(vkCreateImageView(device, &imageCreateViewInfo, NO_ALLOCATOR, &createdImageView));

	VkSamplerCreateInfo samplerCreate = 
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 0,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = 0.0f
	};

	VkSampler imageSampler;
	VK_CHECK(vkCreateSampler(device, &samplerCreate, NO_ALLOCATOR, &imageSampler));

	return (mist_VkImage) { .alloc = imageAlloc, .image = createdImage, .imageView = createdImageView, .sampler = imageSampler};
}

void DestroyVkImage(VkDevice device, mist_VkImage* image)
{
	vkDestroySampler(device, image->sampler, NO_ALLOCATOR);
	vkDestroyImageView(device, image->imageView, NO_ALLOCATOR);
	vkDestroyImage(device, image->image, NO_ALLOCATOR);
}

mist_VkImage InitFont(
	VkDevice device, 
	mist_PhysicalDevice* physicalDevice,
	mist_VkAllocator* bufferAllocator,
	VkCommandPool commandPool,
	VkQueue commandQueue)
{
	Font_InitIndices();

	int x, y, n;
	stbi_uc* imageData = stbi_load("../../Assets/Images/CourierNew.bmp", &x, &y, &n, STBI_rgb_alpha);

	mist_VkImage image = CreateVkImage(device, physicalDevice, bufferAllocator, imageData, x, y, STBI_rgb_alpha, commandPool, commandQueue);

	stbi_image_free(imageData);
	return image;
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

	SelectBestPhysicalDevice(g_VkSurface, &g_VkSelectedDevice, &g_VkGraphicsQueueIdx, &g_VkPresentQueueIdx);
	CreateVkLogicDevice(g_VkSelectedDevice->device, g_VkGraphicsQueueIdx, g_VkPresentQueueIdx, &g_VkDevice, &g_VkGraphicsQueue, &g_VkPresentQueue);

	CreateVkSemaphores(g_VkDevice, g_AcquireSemaphores, vkConfig_BufferCount);
	CreateVkSemaphores(g_VkDevice, g_FrameCompleteSemaphores, vkConfig_BufferCount);

	CreateVkCommandBuffers(g_VkDevice, g_VkGraphicsQueueIdx, &g_GraphicsCommandPool, g_GraphicsCommandBuffers, vkConfig_BufferCount);
	CreateVkFences(g_VkDevice, g_GraphicsFences, vkConfig_BufferCount);

	CreateVkShaders(g_VkDevice);

	g_VkDescriptorSetLayout = CreateVkDescriptorSetLayout(g_VkDevice);
	CreateVkDescriptorPools(g_VkDevice, g_VkDescriptorPools, vkConfig_BufferCount);
	g_VkPipelineLayout = CreateVkPipelineLayout(g_VkDevice, g_VkDescriptorSetLayout);
	g_VkPipelineCache = CreateVkPipelineCache(g_VkDevice);

	InitVkAllocators(g_VkDevice);
	CreateVkMeshes(g_VkDevice, g_VkSelectedDevice, &g_VkAllocator);

	VkSurfaceFormatKHR surfaceFormat = SelectVkSurfaceFormat(g_VkSelectedDevice);
	g_VkSurfaceExtents = CreateSurfaceExtents(g_VkSelectedDevice, surfaceWidth, surfaceHeight);
	g_VkSwapchain = CreateVkSwapchain(g_VkSelectedDevice, g_VkDevice, g_VkSurface, surfaceFormat, g_VkSurfaceExtents, g_VkGraphicsQueueIdx, g_VkPresentQueueIdx, vkConfig_BufferCount);
	g_VkRenderPass = CreateVkRenderPass(g_VkDevice, surfaceFormat);

	g_VkFontImage = InitFont(g_VkDevice, g_VkSelectedDevice, &g_VkAllocator, g_GraphicsCommandPool, g_VkGraphicsQueue);

	CreateVkFramebuffers(g_VkDevice, g_VkSwapchain, surfaceFormat, g_VkRenderPass, g_VkSurfaceExtents, g_VkFramebuffers, g_VkSwapchainImages, vkConfig_BufferCount);
	g_VkPipeline = CreateVkPipeline(g_VkDevice, g_VkRenderPass, g_VkShaders[VkShader_DefaultVert], g_VkShaders[VkShader_DefaultFrag], g_VkPipelineCache, g_VkPipelineLayout, surfaceWidth, surfaceHeight);
	CreateVkDescriptorSet(g_VkDevice, g_VkSelectedDevice, &g_VkAllocator, surfaceWidth, surfaceHeight, &g_VkFontImage, g_VkDescriptorSetLayout, g_VkDescriptorBuffers, g_VkDescriptorPools, g_VkDescriptorSets, vkConfig_BufferCount);
	RecordCommandBuffers();
}

void VkRenderer_Kill(void)
{
	vkDeviceWaitIdle(g_VkDevice);
	DestroyVkDescriptorSets(g_VkDevice, g_VkDescriptorBuffers, vkConfig_BufferCount);

	DestroyVkImage(g_VkDevice, &g_VkFontImage);
	DestroyVkMeshes(g_VkDevice);
	KillVkAllocators();

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

void VkRenderer_Draw(
	uint32_t surfaceWidth, 
	uint32_t surfaceHeight)
{
	static uint8_t s_CurrentFrame = 0;

	// Start the frame
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(g_VkDevice, g_VkSwapchain, UINT64_MAX, g_AcquireSemaphores[s_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateVkSwapchain(surfaceWidth, surfaceHeight);
		return;
	}

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

	// Present the frame
	VK_CHECK(vkResetFences(g_VkDevice, 1, &g_GraphicsFences[s_CurrentFrame]));

	VK_CHECK(vkQueueSubmit(g_VkGraphicsQueue, 1, &submitInfo, g_GraphicsFences[s_CurrentFrame]));

	VK_CHECK(vkWaitForFences(g_VkDevice, 1, &g_GraphicsFences[s_CurrentFrame], VK_TRUE, UINT64_MAX));

	VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = finished,
		.swapchainCount = 1,
		.pSwapchains = &g_VkSwapchain,
		.pImageIndices = &imageIndex
	};
	VkResult presentResult = vkQueuePresentKHR(g_VkPresentQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
	{
		RecreateVkSwapchain(surfaceWidth, surfaceHeight);
	}

	s_CurrentFrame = (s_CurrentFrame + 1) % vkConfig_BufferCount;
}

void VkRenderer_ClearInstances(void)
{
	for (uint32_t i = 0; i < VkMesh_Count; i++)
	{
		VkDrawIndirectCommand* indirectCommand = (VkDrawIndirectCommand*)g_VkIndirectDrawBuffer.mappedMem + i;
		indirectCommand->instanceCount = 0;
	}
}

void VkRenderer_AddInstance(
	mist_VkMesh mesh, 
	mist_Vec2 position, 
	mist_Vec2 scale, 
	mist_Color color,
	const char* string)
{
	VkDrawIndirectCommand* indirectCommand = (VkDrawIndirectCommand*)g_VkIndirectDrawBuffer.mappedMem + mesh;

	mist_VkInstance* instance = (mist_VkInstance*)g_VkInstances[mesh].mappedMem + indirectCommand->instanceCount;
	instance->position = position;
	instance->scale = scale;
	instance->color = color;
	instance->stringSize = strlen(string);
	assert(instance->stringSize <= 16);
	Font_StringToIndices(string, (uint8_t*)instance->stringBuffer);

	assert(indirectCommand->instanceCount + 1 < vkConfig_MaxInstanceCount);
	indirectCommand->instanceCount++;
}