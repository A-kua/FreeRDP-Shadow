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
			akua_file_process(s,type);
			goto intercept;
		}
		case STREAM_CONTROL:
		{
			akua_control_process(s, type);
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