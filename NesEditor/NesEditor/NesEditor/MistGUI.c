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
	mist_Vec2 labelPos = (mist_Vec2) { .x = roundf(position.x + (size.x - labelSize.x)  * 0.5f), .y = roundf(position.y + (size.y - labelSize.y)  * 0.5f) };

	if (   fabsf(g_MousePosition.x - position.x - size.x * 0.5f) < size.x * 0.5f
		&& fabsf(g_MousePosition.y - position.y - size.y * 0.5f) < size.y * 0.5f)
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
		if (   fabsf(g_MousePosition.x - position.x - innerSize.x * 0.5f) < innerSize.x * 0.5f
			&& fabsf(g_MousePosition.y - position.y - innerSize.y * 0.5f) < innerSize.y * 0.5f)
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

mist_Color ByteToColor(uint8_t* color)
{
	return (mist_Color) { .r = (float)color[0] / 7.0f, .g = (float)color[1] / 7.0f, .b = (float)color[2] / 7.0f, .a = 1.0f };
}

uint8_t NESColors[] =
{
	3,3,3,0,1,4,0,0,6,3,2,6,4,0,3,5,0,3,5,1,0,4,2,0,3,2,0,1,2,0,0,3,1,0,4,0,0,2,2,0,0,0,0,0,0,0,0,0,
	5,5,5,0,3,6,0,2,7,4,0,7,5,0,7,7,0,4,7,0,0,6,3,0,4,3,0,1,4,0,0,4,0,0,5,3,0,4,4,0,0,0,0,0,0,0,0,0,
	7,7,7,3,5,7,4,4,7,6,3,7,7,0,7,7,3,7,7,4,0,7,5,0,6,6,0,3,6,0,0,7,0,2,7,6,0,7,7,0,0,0,0,0,0,0,0,0,
	7,7,7,5,6,7,6,5,7,7,5,7,7,4,7,7,5,5,7,6,4,7,7,2,7,7,3,5,7,2,4,7,3,2,7,6,4,6,7,0,0,0,0,0,0,0,0,0
};

void GUI_NESCanvas(mist_NESCanvas* nesCanvas, mist_NESPixel* nesPixel, mist_Vec2 position, mist_Vec2 size)
{
	mist_Vec2 tileSize = { .x = size.x / (float)nesCanvas->width,.y = size.y / (float)nesCanvas->height };

	uint16_t blockWidth = nesCanvas->width / 8, blockHeight = nesCanvas->height / 8;
	// Larger Block
	for (uint16_t yBlock = 0; yBlock < blockHeight; yBlock++)
	{
		for (uint16_t xBlock = 0; xBlock < blockWidth; xBlock++)
		{
			uint8_t paletteIndex = nesCanvas->paletteBlockIndices[nesCanvas->isForeground][yBlock * blockWidth + xBlock];
			uint8_t* paletteBlock = &nesCanvas->palette[nesCanvas->isForeground][paletteIndex * 4];

			// Inner Block
			for (uint8_t y = 0; y < 8; y++)
			{
				for (uint8_t x = 0; x < 8; x++)
				{
					uint16_t pixelIndex = (yBlock * blockWidth * 64 + xBlock * 8) + (y * 8 * blockWidth) + x;

					uint8_t pixelPaletteIndex = nesCanvas->pixelCanvas[nesCanvas->isForeground][pixelIndex];
					uint8_t* pixelColor;
					pixelColor = &NESColors[paletteBlock[pixelPaletteIndex] * 3];

					uint16_t pixelX = xBlock * 8 + x;
					uint16_t pixelY = yBlock * 8 + y;

					mist_Vec2 pixelPos = { .x = position.x + tileSize.x * pixelX,.y = position.y + tileSize.y * pixelY };

					if (g_MouseState == MouseState_Held || g_MouseState == MouseState_Down)
					{
						if (	fabsf(g_MousePosition.x - pixelPos.x - tileSize.x * 0.5f) < tileSize.x * 0.5f
							&&	fabsf(g_MousePosition.y - pixelPos.y - tileSize.y * 0.5f) < tileSize.y * 0.5f)
						{
							nesCanvas->paletteBlockIndices[nesCanvas->isForeground][yBlock * blockWidth + xBlock] = nesPixel->paletteIndex;
							nesCanvas->pixelCanvas[nesCanvas->isForeground][pixelIndex] = nesPixel->blockColor;

							mist_Color pressedColor = { 0 };
							uint8_t selectedPaletteBlock = nesCanvas->palette[nesCanvas->isForeground][nesPixel->paletteIndex * 4 + nesPixel->blockColor];
							pressedColor = ByteToColor(&NESColors[selectedPaletteBlock * 3]);

							VkRenderer_AddInstance(VkMesh_Rect, pixelPos, tileSize, pressedColor, "");
							continue;
						}
					}

					VkRenderer_AddInstance(VkMesh_Rect, pixelPos, tileSize, ByteToColor(pixelColor), "");
				}
			}
		}
	}
}

void GUI_NESPalette(mist_NESCanvas* nesCanvas, mist_NESPixel* nesPixel, mist_Vec2 position, mist_Vec2 size)
{
	#define NESConfig_Border (mist_Color){ .r = 0.2f, .g = 0.2f, .b = 0.2f, .a = 1.0f }

	mist_Vec2 borderSize = { .x = size.x / 2.0f,.y = size.y / 8.0f };
	mist_Vec2 tileSize = { .x = borderSize.x * 0.9f, .y = borderSize.y * 0.9f };

	mist_Vec2 tilePosition = position;
	
	// Left
	for (uint8_t block = 0; block < 2; block++)
	{
		for (uint8_t pixel = 0; pixel < 4; pixel++)
		{
			uint8_t paletteIndex = block * 4 + pixel;
			bool isCurrentlySelected = nesPixel->paletteIndex * 4 + nesPixel->blockColor == paletteIndex;

			mist_Vec2 borderPosition = (mist_Vec2) { .x = tilePosition.x, .y = tilePosition.y + borderSize.y * (block * 4 + pixel) };
			mist_Vec2 pixelPosition = (mist_Vec2) { .x = borderPosition.x + (borderSize.x - tileSize.x) * 0.5f, .y = borderPosition.y + (borderSize.y - tileSize.y) * 0.5f };
			if (	!isCurrentlySelected
				&&	fabsf(g_MousePosition.x - pixelPosition.x - tileSize.x * 0.5f) < tileSize.x * 0.5f
				&&	fabsf(g_MousePosition.y - pixelPosition.y - tileSize.y * 0.5f) < tileSize.y * 0.5f)
			{
				if (g_MouseState == MouseState_Up)
				{
					nesPixel->paletteIndex = block;
					nesPixel->blockColor = pixel;
				}
				VkRenderer_AddInstance(VkMesh_Rect, borderPosition, borderSize, GUIConfig_Hover, "");
			}
			else
			{
				if (isCurrentlySelected)
				{
					VkRenderer_AddInstance(VkMesh_Rect, borderPosition, borderSize, GUIConfig_White, "");
				}
			}

			VkRenderer_AddInstance(VkMesh_Rect, pixelPosition, tileSize, ByteToColor(&NESColors[nesCanvas->palette[nesCanvas->isForeground][paletteIndex] * 3]), "");
		}
	}

	tilePosition.y = position.y;
	tilePosition.x += borderSize.x;
	// Right
	for (uint8_t block = 0; block < 2; block++)
	{
		for (uint8_t pixel = 0; pixel < 4; pixel++)
		{
			uint8_t paletteIndex = block * 4 + pixel + 8;

			bool isCurrentlySelected = nesPixel->paletteIndex * 4 + nesPixel->blockColor == paletteIndex;

			mist_Vec2 borderPosition = (mist_Vec2) { .x = tilePosition.x, .y = tilePosition.y + borderSize.y * (block * 4 + pixel) };
			mist_Vec2 pixelPosition = (mist_Vec2) { .x = borderPosition.x + (borderSize.x - tileSize.x) * 0.5f, .y = borderPosition.y + (borderSize.y - tileSize.y) * 0.5f };
			if (!isCurrentlySelected
				&&	fabsf(g_MousePosition.x - pixelPosition.x - tileSize.x * 0.5f) < tileSize.x * 0.5f
				&&	fabsf(g_MousePosition.y - pixelPosition.y - tileSize.y * 0.5f) < tileSize.y * 0.5f)
			{
				if (g_MouseState == MouseState_Up)
				{
					nesPixel->paletteIndex = 2 + block;
					nesPixel->blockColor = pixel;
				}
				VkRenderer_AddInstance(VkMesh_Rect, borderPosition, borderSize, GUIConfig_Hover, "");
			}
			else
			{
				if (isCurrentlySelected)
				{
					VkRenderer_AddInstance(VkMesh_Rect, borderPosition, borderSize, GUIConfig_White, "");
				}
			}

			VkRenderer_AddInstance(VkMesh_Rect, pixelPosition, tileSize, ByteToColor(&NESColors[nesCanvas->palette[nesCanvas->isForeground][paletteIndex] * 3]), "");
		}
	}
}

void GUI_NESColorPicker(mist_NESCanvas* nesCanvas, mist_NESPixel* nesPixel, mist_Vec2 position, mist_Vec2 size)
{
	uint16_t nesColorCount = ARRAYSIZE(NESColors) / 3;
	uint8_t rowCount = nesColorCount / 3;

	mist_Vec2 tileSize = { .x = size.x / 3.0f, size.y / ((float)rowCount) };
	mist_Vec2 tilePosition = position;
	for (uint8_t col = 0; col < 3; col++)
	{
		for (uint8_t row = 0; row < rowCount; row++)
		{
			if (g_MouseState == MouseState_Up)
			{
				if (	fabsf(g_MousePosition.x - tilePosition.x - tileSize.x * 0.5f) < tileSize.x * 0.5f
					&&	fabsf(g_MousePosition.y - tilePosition.y - tileSize.y * 0.5f) < tileSize.y * 0.5f)
				{
					if (nesPixel->blockColor == 0)
					{
						for (unsigned int background = 0; background < 2; background++)
						{
							nesCanvas->palette[background][0] = col * rowCount + row;
							nesCanvas->palette[background][4] = col * rowCount + row;
							nesCanvas->palette[background][8] = col * rowCount + row;
							nesCanvas->palette[background][12] = col * rowCount + row;
						}
					}
					else
					{
						nesCanvas->palette[nesCanvas->isForeground][nesPixel->paletteIndex * 4 + nesPixel->blockColor] = col * rowCount + row;
					}
				}
			}

			VkRenderer_AddInstance(VkMesh_Rect, tilePosition, tileSize, ByteToColor(&NESColors[(col * rowCount + row) * 3]), "");

			tilePosition.y += tileSize.y;
		}
		tilePosition.x += tileSize.x;
		tilePosition.y = position.y;
	}
}
