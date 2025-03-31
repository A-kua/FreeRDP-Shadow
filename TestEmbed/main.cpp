#include <stdint.h>
#include <stdio.h>

typedef uint8_t UINT8;

typedef uint16_t UINT16;

typedef uint32_t UINT32;

struct _rgb
{
	UINT8 r;
	UINT8 g;
	UINT8 b;
};
typedef struct _rgb rgb;

UINT16 Uint16BE(UINT8 d0, UINT16 d1) {
	return (UINT16)d0 << 8 | d1;
}

void PutUint16BE(UINT16 data, UINT8* d0, UINT8* d1) {
	*d0 = (UINT8)(data >> 8);
	*d1 = (UINT8)(data & 0xFF);
}

rgb RGB565ToRGB(UINT16 data) {
	rgb rgb;
	rgb.r = (data & 0xF800) >> 8;
	rgb.g = (data & 0x07E0) >> 3;
	rgb.b = (data & 0x001F) << 3;
	return rgb;
}


rgb RGB555ToRGB(UINT16 data) {
	rgb rgb;
	rgb.r = (data & 0x7C00) >> 7;
	rgb.g = (data & 0x03E0) >> 2;
	rgb.b = (data & 0x001F) << 3;
	return rgb;
}

UINT16 RGBToRGB565(UINT16 r, UINT16 g, UINT16 b) {
	UINT16 r5 = ((UINT16)(r >> 3)) << 11;
	UINT16 g6 = ((UINT16)(g >> 2)) << 5;
	UINT16 b5 = (UINT16)(b >> 3);
	return r5 | g6 | b5;
}

UINT16 RGBToRGB555(UINT16 r, UINT16 g, UINT16 b) {
	UINT16 r5 = ((UINT16)(r >> 3)) << 10;
	UINT16 g5 = ((UINT16)(g >> 3)) << 5;
	UINT16 b5 = (UINT16)(b >> 3);
	return r5 | g5 | b5;
}

void FromRGB(UINT32 bitsPerPixel, rgb rgb, int i, UINT8* data) {
	switch (bitsPerPixel)
	{
	case 1:
	{
		UINT16 rgb555 = RGBToRGB555(rgb.r, rgb.g, rgb.b);
		PutUint16BE(rgb555, &data[i], &data[i + 1]);
		break;
	}
	case 2:
	{
		UINT16 rgb565 = RGBToRGB565(rgb.r, rgb.g, rgb.b);
		PutUint16BE(rgb565, &data[i], &data[i + 1]);
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
rgb ToRGB(UINT32 bitsPerPixel, int i, UINT8* data) {
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

void updatePixel(UINT8* pixel, int diff) {
	int newValue = (int)*pixel + diff;
	if (newValue < 0) {
		newValue = 0;
	}
	else if (newValue > 255) {
		newValue = 255;
	}
	*pixel = (UINT8)newValue;
}

void printAsGoArray(const unsigned char* data, size_t length);

int embed_data(UINT8* originImg, UINT32 width, UINT32 height, UINT32 bitsPerPixel, UINT8* data, UINT32 dataLen)
{
	int x, y, i = 0;
	int byteIndex = 0, bitIndex = 0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			rgb rgb = ToRGB(bitsPerPixel, i, originImg);

			if (byteIndex >= dataLen) {
				printAsGoArray(originImg, 32);
				return 0;
			}
			UINT8 currentByte = data[byteIndex];
			UINT8 bitB = (currentByte >> (7 - bitIndex)) & 1;
			UINT8 bitR = (currentByte >> (7 - (bitIndex + 1))) & 1;

			UINT8 b = rgb.b;
			UINT8 r = rgb.r;

			UINT8 bHigh5 = (b >> 3) & 0x1F;
			UINT8 rHigh5 = (r >> 3) & 0x1F;

			UINT8 bDiff = bHigh5 % 2;
			UINT8 rDiff = rHigh5 % 2;

			if ((bitB == 0 && bDiff != 0) || (bitB == 1 && bDiff == 0)) {
				if (bHigh5 > 0) {
					updatePixel(&b, -1 << 3);
				}
				else {
					updatePixel(&b, 1 << 3);
				}
			}

			if ((bitR == 0 && rDiff != 0) || (bitR == 1 && rDiff == 0)) {
				if (rHigh5 > 0) {
					updatePixel(&r, -1 << 3);
				}
				else {
					updatePixel(&r, 1 << 3);
				}
			}
			rgb.b = b;
			rgb.r = r;

			FromRGB(bitsPerPixel, rgb, i, originImg);

			bitIndex += 2;
			if (bitIndex >= 8) {
				bitIndex = 0;
				byteIndex++;
			}
			i += bitsPerPixel;
		}
	}
	return 0;
}


void printAsGoArray(const unsigned char* data, size_t length) {
	printf("[");
	for (size_t i = 0; i < length; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("0x%02X", data[i]);
	}
	printf("]\n");
}

int main() {
	unsigned char data[32] = { 8, 17, 95, 223, 0x11, 0x08, 0xDF, 0x5F, 0x01, 0x0A, 0x01, 0x0A, 0x01, 0x0A, 0x01, 0x0A, 0xDF, 0x5F, 0x11, 0x08, 0x01, 0x0A, 0x01, 0x0A, 0x01, 0x09, 0xCF, 0x5F, 0xCF, 0x5F, 0x01, 0x09 };
	
	unsigned char info[4] = { 0xde,0xad,0x00,0x01 };
	printAsGoArray(data, 32);
	embed_data(data, 4, 4, 2, info, 4);
	printAsGoArray(data, 32);
	return 0;
}