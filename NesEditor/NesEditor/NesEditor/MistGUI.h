#pragma once

#include "VkRenderer.h"

#include <stdbool.h>

typedef enum mist_MouseState
{
	MouseState_None,
	MouseState_Down,
	MouseState_Held,
	MouseState_Up
} mist_MouseState;

extern mist_Vec2 g_MousePosition;
extern mist_MouseState g_MouseState;

bool GUI_Button(mist_Vec2 position, mist_Vec2 size, const char* string);
bool GUI_CheckBox(bool state, mist_Vec2 position, mist_Vec2 size);
void GUI_Label(mist_Vec2 position, mist_Vec2 size, const char* string);

int8_t GUI_Toolbar(mist_Vec2 position, mist_Vec2 tabSize, const char** tabs, int8_t tabCount);
