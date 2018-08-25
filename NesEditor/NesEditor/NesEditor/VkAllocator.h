#ifndef __VK_ALLOCATOR_H
#define __VK_ALLOCATOR_H

#include <vulkan/vulkan.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct mist_VkMemBlock
{
	VkDeviceSize size;
	VkDeviceSize offset;
	bool         allocated;
	int          id;

	struct mist_VkMemBlock* next;

} mist_VkMemBlock;

typedef enum mist_AllocatorFlags
{
	mist_AllocatorNone = 0x00,
	mist_AllocatorHostVisible = 0x01

} mist_AllocatorFlags;

typedef struct mist_VkAllocatorPool
{
	mist_VkMemBlock* head;

	VkDeviceMemory   memory;
	VkDeviceSize     size;
	int              nextId;

	uint32_t         memoryType;
	mist_AllocatorFlags flags;

	void*            mappedMem;

} mist_VkAllocatorPool;

#define vkConfig_MaxVkAllocatorPool 10
typedef struct mist_VkAllocator
{
	VkDevice device;
	mist_VkAllocatorPool pools[vkConfig_MaxVkAllocatorPool];
} mist_VkAllocator;

typedef struct mist_VkAlloc
{
	VkDeviceMemory memory;
	VkDeviceSize   offset;
	int            id;

	mist_VkAllocatorPool* pool;
} mist_VkAlloc;

void mist_InitVkAllocator(mist_VkAllocator* allocator, VkDevice device);
void mist_CleanupVkAllocator(mist_VkAllocator* allocator);

mist_VkAlloc mist_VkAllocate(mist_VkAllocator* allocator, uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment, mist_AllocatorFlags flags);
void mist_VkFree(mist_VkAlloc allocation);

void* mist_VkMapMemory(mist_VkAlloc allocation);

#endif //__ALLOCATOR_H