#ifndef __VK_RENDERER_H
#define __VK_RENDERER_H

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

typedef enum mist_VkMesh
{
	VkMesh_Rect = 0,
	VkMesh_Count
} mist_VkMesh;

typedef struct mist_Vec2
{
	float x, y;
} mist_Vec2;

typedef struct mist_Color
{
	float r, g, b, a;
} mist_Color;

void VkRenderer_Init(HINSTANCE appInstance, HWND window, uint32_t surfaceWidth, uint32_t surfaceHeight);
void VkRenderer_Kill(void);

void VkRenderer_Draw(uint32_t surfaceWidth, uint32_t surfaceHeight);

void VkRenderer_ClearInstances(void);
void VkRenderer_AddInstance(mist_VkMesh mesh, mist_Vec2 position, mist_Vec2 scale, mist_Color color, const char* string);


#endif // __VK_RENDERER_H
