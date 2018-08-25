#include "MistGUI.h"

#include "VkRenderer.h"

#include <math.h>

#define GUIConfig_Unselected (mist_Color){ .r = 0.2f, .g = 0.2f, .b = 0.2f, .a = 1.0f }
#define GUIConfig_Hover (mist_Color){ .r = 0.3f, .g = 0.3f, .b = 0.3f, .a = 1.0f }
#define GUIConfig_Selected (mist_Color){ .r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f }
#define GUIConfig_White (mist_Color){ .r = 0.9f, .g = 0.9f, .b = 0.9f, .a = 1.0f }

mist_Vec2 g_MousePosition;
mist_MouseState g_MouseState;

bool GUI_Button(mist_Vec2 position, mist_Vec2 size, const char* string)
{
	mist_Vec2 labelSize = (mist_Vec2) { .x = (float)strlen(string) * 10.0f, .y = 18.0f };
	mist_Vec2 labelPos = (mist_Vec2) { .x = roundf(position.x), .y = roundf(position.y) };

	if (   fabsf(g_MousePosition.x - position.x) < size.x * 0.5f
		&& fabsf(g_MousePosition.y - position.y) < size.y * 0.5f)
	{
		if (g_MouseState != MouseState_None)
		{
			VkRenderer_AddInstance(VkMesh_Rect, position, size, GUIConfig_Selected, "");
			GUI_Label(labelPos, labelSize, string);
			return g_MouseState == MouseState_Up;
		}
		else
		{
			VkRenderer_AddInstance(VkMesh_Rect, position, size, GUIConfig_Hover, "");
			GUI_Label(labelPos, labelSize, string);
			return false;
		}
	}

	VkRenderer_AddInstance(VkMesh_Rect, position, size, GUIConfig_Unselected, "");
	GUI_Label(labelPos, labelSize, string);
	return false;
}

bool GUI_CheckBox(bool state, mist_Vec2 position, mist_Vec2 size)
{
	VkRenderer_AddInstance(VkMesh_Rect, position, size, GUIConfig_Selected, "");

	mist_Vec2 innerSize = (mist_Vec2){ .x = size.x * 0.9f, .y = size.y * 0.9f };
	if (g_MouseState == MouseState_Up)
	{
		if (   fabsf(g_MousePosition.x - position.x) < innerSize.x * 0.5f
			&& fabsf(g_MousePosition.y - position.y) < innerSize.y * 0.5f)
		{
			state = !state;
		}
	}

	VkRenderer_AddInstance(VkMesh_Rect, position, innerSize, GUIConfig_Unselected, "");

	mist_Vec2 buttonSize = (mist_Vec2) { .x = innerSize.x * 0.95f, .y = innerSize.y * 0.95f };
	if (state)
	{
		VkRenderer_AddInstance(VkMesh_Rect, position, buttonSize, GUIConfig_Selected, "");
	}
	else
	{
		VkRenderer_AddInstance(VkMesh_Rect, position, buttonSize, GUIConfig_Unselected, "");
	}

	return state;
}

void GUI_Label(mist_Vec2 position, mist_Vec2 size, const char* string)
{
	VkRenderer_AddInstance(VkMesh_Rect, position, size, GUIConfig_White, string);
}

int8_t GUI_Toolbar(mist_Vec2 position, mist_Vec2 tabSize, const char** tabs, int8_t tabCount)
{
	int8_t selectedTab = -1;
	for (int8_t i = 0; i < tabCount; i++)
	{
		mist_Vec2 tabPos = position;
		tabPos.x += tabSize.x * i;

		if (GUI_Button(tabPos, tabSize, tabs[i]))
		{
			selectedTab = i;
		}
	}

	return selectedTab;
}
