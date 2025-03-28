#include "akua_config.h"

rgb RGB565ToRGB(UINT16 data)
{
	rgb rgb;
	rgb.r = (data & 0xF800) >> 8;
	rgb.g = (data & 0x07E0) >> 3;
	rgb.b = (data & 0x001F) << 3;
	return rgb;
}

rgb RGB555ToRGB(UINT16 data)
{
	rgb rgb;
	rgb.r = (data & 0x7C00) >> 7;
	rgb.g = (data & 0x03E0) >> 2;
	rgb.b = (data & 0x001F) << 3;
	return rgb;
}

UINT16 RGBToRGB565(UINT16 r, UINT16 g, UINT16 b)
{
	UINT16 r5 = ((UINT16)(r >> 3)) << 11;
	UINT16 g6 = ((UINT16)(g >> 2)) << 5;
	UINT16 b5 = (UINT16)(b >> 3);
	return r5 | g6 | b5;
}

UINT16 RGBToRGB555(UINT16 r, UINT16 g, UINT16 b)
{
	UINT16 r5 = ((UINT16)(r >> 3)) << 10;
	UINT16 g5 = ((UINT16)(g >> 3)) << 5;
	UINT16 b5 = (UINT16)(b >> 3);
	return r5 | g5 | b5;
}

void FromRGB(UINT32 bitsPerPixel, rgb rgb, int i, UINT8* data)
{
	switch (bitsPerPixel)
	{
		case 1:
		{
			UINT16 rgb555 = RGBToRGB555(rgb.r, rgb.g, rgb.b);
			PutUint16BE(rgb555, &data[i+1], &data[i ]);
			break;
		}
		case 2:
		{
			UINT16 rgb565 = RGBToRGB565(rgb.r, rgb.g, rgb.b);
			PutUint16BE(rgb565, &data[i+1], &data[i]);
			break;
		}
		default:
		{
			data[i] = rgb.b;
			data[i + 1] = rgb.g;
			data[i + 2] = rgb.r;
			break;
		}
	}
}
rgb ToRGB(UINT32 bitsPerPixel, int i, UINT8* data)
{
	rgb rgb;
	switch (bitsPerPixel)
	{
		case 1:
		{
			UINT16 rgb555 = Uint16BE(data[i], data[i + 1]);
			rgb = RGB555ToRGB(rgb555);
			break;
		}
		case 2:
		{
			UINT16 rgb565 = Uint16BE(data[i], data[i + 1]);
			rgb = RGB565ToRGB(rgb565);
			break;
		}
		default:
		{
			rgb.r = data[i + 2];
			rgb.g = data[i + 1];
			rgb.b = data[i];
			break;
		}
	}
	return rgb;
}

UINT32 BPP(UINT32 bitsPerPixel)
{
	switch (bitsPerPixel)
	{
		case 15:
			return 1;
		case 16:
			return 2;
		case 24:
			return 3;
		default:
			return 4;
	}
}