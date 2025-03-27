#include "akua_config.h"

static BOOL controlState = TRUE;
void akua_set_control(BOOL state)
{
	controlState = state;
}
BOOL akua_shadow_input_synchronize_event(rdpInput* input, UINT32 flags,
                                         ptr_shadow_input_synchronize_event origin)
{
	if (controlState)
		return origin(input, flags);
	return TRUE;
}
BOOL akua_shadow_input_keyboard_event(rdpInput* input, UINT16 flags, UINT8 code,
                                      ptr_shadow_input_keyboard_event origin)
{
	if (controlState)
		return origin(input, flags, code);
	return TRUE;
}
BOOL akua_shadow_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code,
                                              ptr_shadow_input_unicode_keyboard_event origin)
{
	if (controlState)
		return origin(input, flags, code);
	return TRUE;
}
BOOL akua_shadow_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y,
                                   ptr_shadow_input_mouse_event origin)
{
	if (controlState)
		return origin(input, flags, x, y);
	return TRUE;
}
BOOL akua_shadow_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y,
                                            ptr_shadow_input_extended_mouse_event origin)
{
	if (controlState)
		return origin(input, flags, x, y);
	return TRUE;
}