#include "VkAllocator.h"

#include "MistVk.h"

#include <stdlib.h>

#define VALIDATE_MEMORY(allocator, allocation) assert(allocator->memory == allocation.memory)

VkDeviceSize mist_ClosestPowerOfTwo(VkDeviceSize size)
{
	VkDeviceSize closest = 1;
	while (closest < size)
	{
		closest = closest << 1;
	}
	return closest;
}

void mist_InitVkAllocator(mist_VkAllocator* allocator, VkDevice device, VkDeviceSize size, uint32_t memoryTypeIndex, mist_AllocatorFlags flags)
{
	allocator->size = mist_ClosestPowerOfTwo(size);
	allocator->flags = flags;

	VkMemoryAllocateInfo memoryAllocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = allocator->size, .memoryTypeIndex = memoryTypeIndex };

	VK_CHECK(vkAllocateMemory(device, &memoryAllocInfo, NO_ALLOCATOR, &allocator->memory));

	if ((allocator->flags & mist_AllocatorHostVisible) != 0)
	{
		VK_CHECK(vkMapMemory(device, allocator->memory, 0, allocator->size, 0, &allocator->mappedMem));
	}

	mist_VkMemBlock* block = calloc(1, sizeof(mist_VkMemBlock));
	block->size = allocator->size;
	block->offset = 0;

	allocator->head = block;
	allocator->nextId = 1;
}

void mist_CleanupVkAllocator(mist_VkAllocator* allocator, VkDevice device)
{
	mist_VkMemBlock* iter = allocator->head;
	while (NULL != iter)
	{
		mist_VkMemBlock* next = iter->next;
		free(iter);
		iter = next;
	}

	vkFreeMemory(device, allocator->memory, NO_ALLOCATOR);
}

mist_VkAlloc mist_VkAllocate(mist_VkAllocator* allocator, VkDeviceSize size, VkDeviceSize alignment)
{
	VkDeviceSize sizeMod = size % alignment;
	VkDeviceSize realSize = sizeMod == 0 ? size : size + alignment - sizeMod;

	VkDeviceSize blockSize = mist_ClosestPowerOfTwo(realSize);

	for (mist_VkMemBlock* iter = allocator->head; NULL != iter; iter = iter->next)
	{
		if (!iter->allocated && iter->size == blockSize)
		{
			iter->allocated = true;
			return (mist_VkAlloc) { .memory = allocator->memory, .offset = iter->offset, .id = iter->id };
		}
	}

	// Couldn't find a block the right size, create one from our closest block
	mist_VkMemBlock* smallestBlock = NULL;
	for (mist_VkMemBlock* iter = allocator->head; NULL != iter; iter = iter->next)
	{
		if (   NULL == smallestBlock
			|| (iter->size < smallestBlock->size && !iter->allocated))
		{
			smallestBlock = iter;
		}
	}

	mist_VkMemBlock* iter = smallestBlock;
	if (NULL != iter)
	{
		while (iter->size != blockSize)
		{
			VkDeviceSize newBlockSize = iter->size / 2;

			iter->allocated = true;

			mist_VkMemBlock* left = calloc(1, sizeof(mist_VkMemBlock));
			left->offset = iter->offset;
			left->size = newBlockSize;
			left->id = allocator->nextId;

			++allocator->nextId;


			mist_VkMemBlock* right = calloc(1, sizeof(mist_VkMemBlock));
			right->offset = iter->offset + newBlockSize;
			right->size = newBlockSize;
			right->id = allocator->nextId;

			++allocator->nextId;


			left->next = right;
			right->next = iter->next;
			iter->next = left;

			iter = left;
		}

		iter->allocated = true;
		return (mist_VkAlloc) { .memory = allocator->memory, .offset = iter->offset, .id = iter->id };
	}

	printf("Error: Failed to allocate memory! Allocate more memory!");
	assert(false);

	return (mist_VkAlloc) { 0 };
}

void mist_VkFree(mist_VkAllocator* allocator, mist_VkAlloc allocation)
{
	VALIDATE_MEMORY(allocator, allocation);

	mist_VkMemBlock* prevIters[2] = { NULL };
	mist_VkMemBlock* iter = allocator->head;

	for (; NULL != iter; iter = iter->next)
	{
		if (iter->id == allocation.id)
		{
			iter->allocated = false;

			if (prevIters[0] != NULL)
			{
				// Previous iterator is my size, it's my sibling. If it's not allocated, merge it
				if (prevIters[0]->size == iter->size)
				{
					if (!prevIters[0]->allocated)
					{
						prevIters[1]->allocated = false;
						prevIters[1]->next = iter->next;

						free(iter);
						free(prevIters[0]);
					}
				}
				// Since we just checked to see if the previous iterator was our sibling and it wasnt
				// we know that if we have a next iterator, it's our sibling
				else if (iter->next != NULL)
				{
					if (!iter->next->allocated)
					{
						prevIters[0]->allocated = false;
						prevIters[0]->next = iter->next->next;

						free(iter->next);
						free(iter);
					}
				}
			}
			break;
		}

		prevIters[1] = prevIters[0];
		prevIters[0] = iter;
	}
}

void* mist_VkMapMemory(mist_VkAllocator* allocator, mist_VkAlloc allocation)
{
	VALIDATE_MEMORY(allocator, allocation);
	assert((allocator->flags & mist_AllocatorHostVisible) != 0);

	return (uint8_t*)allocator->mappedMem + allocation.offset;
}

