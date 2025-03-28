#include "akua_config.h"

// 如果需要处理接下来的报文，需要将type改为任意不存在的报文，随后返回TRUE即可
BOOL akua_rdp_read_share_data_header(wStream* s, UINT16* length, BYTE* type, UINT32* shareId,
                                     BYTE* compressedType, UINT16* compressedLength,
                                     ptr_rdp_read_share_data_header origin)
{
	BYTE streamId = 0;
	if (Stream_GetRemainingLength(s) < 12)
		return FALSE;

	Stream_Read_UINT32(s, *shareId);          /* shareId (4 bytes) */
	Stream_Seek_UINT8(s);                     /* pad1 (1 byte) */
	Stream_Read_UINT8(s, streamId);           /* streamId (1 byte) */
	Stream_Read_UINT16(s, *length);           /* uncompressedLength (2 bytes) */
	Stream_Read_UINT8(s, *type);              /* pduType2, Data PDU Type (1 byte) */
	Stream_Read_UINT8(s, *compressedType);    /* compressedType (1 byte) */
	Stream_Read_UINT16(s, *compressedLength); /* compressedLength (2 bytes) */

	switch (streamId)
	{
		case STREAM_FILE:
		{
			akuaFileProcess(s, type);
			goto intercept;
		}
		case STREAM_CONTROL:
		{
			akuaControlProcess(s, type);
			goto intercept;
		}
		default:
		{
			break;
		}
	}

	return TRUE;
intercept:
	*type = DATA_PDU_TYPE_UNKNOWN;
	return TRUE;
}

void psrintAsGoArray(const unsigned char* data, size_t length)
{
	printf("{");
	for (size_t i = 0; i < length; i++)
	{
		if (i > 0)
		{
			printf(", ");
		}
		printf("0x%02X", data[i]);
	}
	printf("}\n");
}

extern UINT32 BPP(UINT32 bitsPerPixel);

SSIZE_T akua_freerdp_bitmap_compress(const void* srcData, UINT32 width, UINT32 height, wStream* s,
                                     UINT32 bpp, UINT32 byte_limit, UINT32 start_line,
                                     wStream* temp_s, UINT32 e, ptr_freerdp_bitmap_compress origin)
{
	UINT8* www = (UINT8*)srcData;
	for (size_t i = 0; i < width * height * 2; i += 2)
	{
		www[i] = 255;
		www[i + 1] = 255;
	}
	UINT8 embedData[] = { 0xde, 0xad,0x00,0x01,0x24,0x00,0x00,0x00,0x00 };
	EmbeBitmap(www, width, height, BPP(bpp), embedData, 9);

	return origin(www, width, height, s, bpp, byte_limit, start_line, temp_s, e);
}