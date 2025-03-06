#include <freerdp/server/shadow.h>

BOOL hook_shadow_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y,
                                          BOOL back) {
	return TRUE;
}