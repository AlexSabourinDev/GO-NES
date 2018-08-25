#pragma once

#include <stdint.h>
#include <string.h>

#include <Windows.h>

static char mist_FontFormat[] = "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static uint8_t mist_FontIndices[255] = { 0 };

static uint8_t mist_FontTileWidth = 10;
static uint8_t mist_FontTileHeight = 18;

static uint16_t mist_FontWidth = 256;
static uint16_t mist_FontHeight = 256;

static void Font_InitIndices()
{
	for (uint32_t i = 0; i < ARRAYSIZE(mist_FontFormat); i++)
	{
		mist_FontIndices[(int)mist_FontFormat[i]] = i;
	}
}

static void Font_StringToIndices(const char* source, uint8_t* buffer)
{
	for (uint8_t i = 0; i < strlen(source); i++)
	{
		buffer[i] = mist_FontIndices[(uint8_t)source[i]];
	}
}
