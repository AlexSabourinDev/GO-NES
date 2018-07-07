#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#include <vulkan/vulkan.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct mist_VkAlloc
{
	VkDeviceMemory memory;
	VkDeviceSize   offset;
	int            id;

} mist_VkAlloc;

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
	mist_AllocatorHostVisible = 0x01

} mist_AllocatorFlags;

typedef struct mist_VkAllocator
{
	mist_VkMemBlock* head;

	VkDeviceMemory   memory;
	VkDeviceSize     size;
	int              nextId;

	mist_AllocatorFlags flags;

	void*            mappedMem;

} mist_VkAllocator;

void mist_InitVkAllocator(mist_VkAllocator* allocator, VkDevice device, VkDeviceSize size, uint32_t memoryTypeIndex, mist_AllocatorFlags flags);
void mist_CleanupVkAllocator(mist_VkAllocator* allocator, VkDevice device);

mist_VkAlloc mist_VkAllocate(mist_VkAllocator* allocator, VkDeviceSize size, VkDeviceSize alignment);
void mist_VkFree(mist_VkAllocator* allocator, mist_VkAlloc allocation);

void* mist_VkMapMemory(mist_VkAllocator* allocator, mist_VkAlloc allocation);

#endif //__ALLOCATOR_H