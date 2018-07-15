#ifndef __MIST_VK_H
#define __MIST_VK_H

#include <vulkan/vulkan.h>

#include <assert.h>
#include <stdio.h>

#include <intrin.h>

#define NO_ALLOCATOR NULL

#define VK_CHECK(call) \
	if(VK_SUCCESS != call) \
	{ \
		printf("Error: Vulkan call failed! File: %s, Line: %d, Function: %s", __FILE__, __LINE__, __func__); \
		__debugbreak(); \
	}

#endif //__MIST_VK_H