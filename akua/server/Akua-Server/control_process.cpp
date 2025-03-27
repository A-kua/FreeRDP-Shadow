#include "akua_config.h"

void akua_control_process(wStream* s, BYTE* type)
{
	UINT16 magicCode1;
	UINT8 state;
	UINT8 magicCode2;
	Stream_Read_UINT16(s, magicCode1);
	if (magicCode1 != 0x8135)
		goto seek_remaining;
	Stream_Read_UINT8(s, state);
	switch (state)
	{
		case 0xff:
		{
			akua_set_control(TRUE);
			break;
		}
		case 0x00:
		{
			akua_set_control(FALSE);
			break;
		}
		default:
			goto seek_remaining;
	}
	Stream_Read_UINT8(s, magicCode2);
	if (magicCode2 != 0xb3)
		goto seek_remaining;
	return;
seek_remaining:
	WLog_ERR("akua", "seek remaining");
	Stream_Seek(s, Stream_GetRemainingLength(s));
}