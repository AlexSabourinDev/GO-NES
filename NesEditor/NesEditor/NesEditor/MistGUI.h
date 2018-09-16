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

typedef struct mist_NESCanvas
{
	bool isForeground;

	uint8_t pixelCanvas[2][128 * 128];
	uint8_t paletteBlockIndices[2][16 * 16];

	uint8_t palette[2][16];

	uint16_t width;
	uint16_t height;

} mist_NESCanvas;

typedef struct mist_NESPixel
{
	uint8_t paletteIndex;
	uint8_t blockColor;
} mist_NESPixel;

extern mist_Vec2 g_MousePosition;
extern mist_MouseState g_MouseState;

bool GUI_Button(mist_Vec2 position, mist_Vec2 size, const char* string);
bool GUI_CheckBox(bool state, mist_Vec2 position, mist_Vec2 size);
void GUI_Label(mist_Vec2 position, mist_Vec2 size, const char* string);

int8_t GUI_Toolbar(mist_Vec2 position, mist_Vec2 tabSize, const char** tabs, int8_t tabCount);
void GUI_NESCanvas(mist_NESCanvas* nesCanvas, mist_NESPixel* nesPixel, mist_Vec2 position, mist_Vec2 size);
void GUI_NESPalette(mist_NESCanvas* nesCanvas, mist_NESPixel* nesPixel, mist_Vec2 position, mist_Vec2 size);
void GUI_NESColorPicker(mist_NESCanvas* nesCanvas, mist_NESPixel* nesPixel, mist_Vec2 position, mist_Vec2 size);