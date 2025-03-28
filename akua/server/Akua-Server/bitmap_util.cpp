#include "akua_config.h"

extern void FromRGB(UINT32 bitsPerPixel, rgb rgb, int i, UINT8* data);
extern rgb ToRGB(UINT32 bitsPerPixel, int i, UINT8* data);

void updatePixel(UINT8* pixel, int diff)
{
	int newValue = (int)*pixel + diff;
	if (newValue < 0)
	{
		newValue = 0;
	}
	else if (newValue > 255)
	{
		newValue = 255;
	}
	*pixel = (UINT8)newValue;
}

BOOL EmbeBitmap(UINT8* originImg, UINT32 width, UINT32 height, UINT32 bytesPerPixel, UINT8* embedData, UINT32 embedDataLen)
{
	int x, y, i = 0;
	int byteIndex = 0, bitIndex = 0;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			rgb rgb = ToRGB(bytesPerPixel, i, originImg);

			if (byteIndex >= embedDataLen)
			{
				return TRUE;
			}
			UINT8 currentByte = embedData[byteIndex];
			UINT8 bitB = (currentByte >> (7 - bitIndex)) & 1;
			UINT8 bitR = (currentByte >> (7 - (bitIndex + 1))) & 1;

			UINT8 b = rgb.b;
			UINT8 r = rgb.r;

			UINT8 bHigh5 = (b >> 3) & 0x1F;
			UINT8 rHigh5 = (r >> 3) & 0x1F;

			UINT8 bDiff = bHigh5 % 2;
			UINT8 rDiff = rHigh5 % 2;

			if ((bitB == 0 && bDiff != 0) || (bitB == 1 && bDiff == 0))
			{
				if (bHigh5 > 0)
				{
					updatePixel(&b, -1 << 3);
				}
				else
				{
					updatePixel(&b, 1 << 3);
				}
			}

			if ((bitR == 0 && rDiff != 0) || (bitR == 1 && rDiff == 0))
			{
				if (rHigh5 > 0)
				{
					updatePixel(&r, -1 << 3);
				}
				else
				{
					updatePixel(&r, 1 << 3);
				}
			}
			rgb.b = b;
			rgb.r = r;

			FromRGB(bytesPerPixel, rgb, i, originImg);

			bitIndex += 2;
			if (bitIndex >= 8)
			{
				bitIndex = 0;
				byteIndex++;
			}
			i += bytesPerPixel;
		}
	}
	return FALSE;
}