#include "akua_config.h"

UINT16 Uint16BE(UINT8 d0, UINT16 d1)
{
	return (UINT16)d0 << 8 | d1;
}

void PutUint16BE(UINT16 data, UINT8* d0, UINT8* d1)
{
	*d0 = (UINT8)(data >> 8);
	*d1 = (UINT8)(data & 0xFF);
}

void PutUint32BE(UINT32 data, UINT8* d0, UINT8* d1, UINT8* d2, UINT8* d3)
{
	*d0 = (UINT8)(data >> 24);
	*d1 = (UINT8)(data >> 16);
	*d2 = (UINT8)(data >> 8);
	*d3 = (UINT8)(data);
}
void AppendU32BigEndian(UINT8* data, size_t length, UINT32 value)
{
	data[length] = (UINT8)(value >> 24);
	data[length + 1] = (UINT8)(value >> 16);
	data[length + 2] = (UINT8)(value >> 8);
	data[length + 3] = (UINT8)(value);
}

void AppendU32LittleEndian(UINT8* data, size_t length, UINT32 value)
{
	data[length] = (UINT8)(value);
	data[length + 1] = (UINT8)(value >> 8);
	data[length + 2] = (UINT8)(value >> 16);
	data[length + 3] = (UINT8)(value >> 24);
}