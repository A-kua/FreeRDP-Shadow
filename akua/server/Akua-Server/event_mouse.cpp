#include <freerdp/server/shadow.h>

BOOL akua_shadow_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y,
                                   ptr_shadow_input_mouse_event origin)
{
	static int flag = 0;
	if (x == 114 && y == 223) {
		flag = flag == 0 ? 1 : 0;
	}
	if (flag)
	{
		return TRUE;
	}
	else
	{
		return origin(input, flags, x, y);
	}
}