// Patched by Simon at 2025/03/27
/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shadow.h"

static BOOL shadow_input_synchronize_event(rdpInput* input, UINT32 flags)
{
	rdpShadowClient* client = (rdpShadowClient*)input->context;
	rdpShadowSubsystem* subsystem = client->server->subsystem;

	if (!client->mayInteract)
		return TRUE;

	return IFCALLRESULT(TRUE, subsystem->SynchronizeEvent, subsystem, client, flags);
}

static BOOL shadow_input_keyboard_event(rdpInput* input, UINT16 flags, UINT8 code)
{
	rdpShadowClient* client = (rdpShadowClient*)input->context;
	rdpShadowSubsystem* subsystem = client->server->subsystem;

	if (!client->mayInteract)
		return TRUE;

	return IFCALLRESULT(TRUE, subsystem->KeyboardEvent, subsystem, client, flags, code);
}

static BOOL shadow_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	rdpShadowClient* client = (rdpShadowClient*)input->context;
	rdpShadowSubsystem* subsystem = client->server->subsystem;

	if (!client->mayInteract)
		return TRUE;

	return IFCALLRESULT(TRUE, subsystem->UnicodeKeyboardEvent, subsystem, client, flags, code);
}

static BOOL shadow_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	rdpShadowClient* client = (rdpShadowClient*)input->context;
	rdpShadowSubsystem* subsystem = client->server->subsystem;

	if (client->server->shareSubRect)
	{
		x += client->server->subRect.left;
		y += client->server->subRect.top;
	}

	if (!(flags & PTR_FLAGS_WHEEL))
	{
		client->pointerX = x;
		client->pointerY = y;

		if ((client->pointerX == subsystem->pointerX) && (client->pointerY == subsystem->pointerY))
		{
			flags &= ~PTR_FLAGS_MOVE;

			if (!(flags & (PTR_FLAGS_BUTTON1 | PTR_FLAGS_BUTTON2 | PTR_FLAGS_BUTTON3)))
				return TRUE;
		}
	}

	if (!client->mayInteract)
		return TRUE;

	return IFCALLRESULT(TRUE, subsystem->MouseEvent, subsystem, client, flags, x, y);
}

static BOOL shadow_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	rdpShadowClient* client = (rdpShadowClient*)input->context;
	rdpShadowSubsystem* subsystem = client->server->subsystem;

	if (client->server->shareSubRect)
	{
		x += client->server->subRect.left;
		y += client->server->subRect.top;
	}

	client->pointerX = x;
	client->pointerY = y;

	if (!client->mayInteract)
		return TRUE;

	return IFCALLRESULT(TRUE, subsystem->ExtendedMouseEvent, subsystem, client, flags, x, y);
}

// Patched by Simon at 2025/03/27
BOOL hooked_shadow_input_synchronize_event_dummy(rdpInput* input, UINT32 flags,
                                                 ptr_shadow_input_synchronize_event origin)
{
	return origin(input, flags);
}
BOOL hooked_shadow_input_keyboard_event_dummy(rdpInput* input, UINT16 flags, UINT8 code,
                                              ptr_shadow_input_keyboard_event origin)
{
	return origin(input, flags, code);
}
BOOL hooked_shadow_input_unicode_keyboard_event_dummy(
    rdpInput* input, UINT16 flags, UINT16 code, ptr_shadow_input_unicode_keyboard_event origin)
{
	return origin(input, flags, code);
}
BOOL hooked_shadow_input_mouse_event_dummy(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y,
                                           ptr_shadow_input_mouse_event origin)
{
	return origin(input, flags, x, y);
}
BOOL hooked_shadow_input_extended_mouse_event_dummy(rdpInput* input, UINT16 flags, UINT16 x,
                                                    UINT16 y,
                                                    ptr_shadow_input_extended_mouse_event origin)
{
	return origin(input, flags, x, y);
}

static int (*ptr_hooked_shadow_input_synchronize_event)(rdpInput*, UINT32,
                                                        ptr_shadow_input_synchronize_event) =
    hooked_shadow_input_synchronize_event_dummy;
static int (*ptr_hooked_shadow_input_keyboard_event)(rdpInput*, UINT16, UINT8,
                                                     ptr_shadow_input_keyboard_event) =
    hooked_shadow_input_keyboard_event_dummy;
static int (*ptr_hooked_shadow_input_unicode_keyboard_event)(
    rdpInput*, UINT16, UINT16,
    ptr_shadow_input_unicode_keyboard_event) = hooked_shadow_input_unicode_keyboard_event_dummy;
static int (*ptr_hooked_shadow_input_mouse_event)(rdpInput*, UINT16, UINT16, UINT16,
                                                  ptr_shadow_input_mouse_event) =
    hooked_shadow_input_mouse_event_dummy;
static int (*ptr_hooked_shadow_input_extended_mouse_event)(rdpInput*, UINT16, UINT16, UINT16,
                                                           ptr_shadow_input_extended_mouse_event) =
    hooked_shadow_input_extended_mouse_event_dummy;

static BOOL hooked_shadow_input_synchronize_event(rdpInput* input, UINT32 flags)
{
	return ptr_hooked_shadow_input_synchronize_event(input, flags, shadow_input_synchronize_event);
}
static BOOL hooked_shadow_input_keyboard_event(rdpInput* input, UINT16 flags, UINT8 code)
{
	return ptr_hooked_shadow_input_keyboard_event(input, flags, code, shadow_input_keyboard_event);
}
static BOOL hooked_shadow_input_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	return ptr_hooked_shadow_input_unicode_keyboard_event(input, flags, code,
	                                                      shadow_input_unicode_keyboard_event);
}
static BOOL hooked_shadow_input_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	return ptr_hooked_shadow_input_mouse_event(input, flags, x, y, shadow_input_mouse_event);
}
static BOOL hooked_shadow_input_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x,
                                                     UINT16 y)
{
	return ptr_hooked_shadow_input_extended_mouse_event(input, flags, x, y,
	                                                    shadow_input_extended_mouse_event);
}

void shadow_input_register_callbacks(rdpInput* input)
{
	input->SynchronizeEvent = hooked_shadow_input_synchronize_event;
	input->KeyboardEvent = hooked_shadow_input_keyboard_event;
	input->UnicodeKeyboardEvent = hooked_shadow_input_unicode_keyboard_event;
	input->MouseEvent = hooked_shadow_input_mouse_event;
	input->ExtendedMouseEvent = hooked_shadow_input_extended_mouse_event;
}

void hook_shadow_input_synchronize_event(int (*ptr)(rdpInput*, UINT32,
                                                    ptr_shadow_input_synchronize_event))
{
	ptr_hooked_shadow_input_synchronize_event = ptr;
}
void hook_shadow_input_keyboard_event(int (*ptr)(rdpInput*, UINT16, UINT8,
                                                 ptr_shadow_input_keyboard_event))
{
	ptr_hooked_shadow_input_keyboard_event = ptr;
}
void hook_shadow_input_unicode_keyboard_event(int (*ptr)(rdpInput*, UINT16, UINT16,
                                                         ptr_shadow_input_unicode_keyboard_event))
{
	ptr_hooked_shadow_input_unicode_keyboard_event = ptr;
}
void hook_shadow_input_mouse_event(int (*ptr)(rdpInput*, UINT16, UINT16, UINT16,
                                              ptr_shadow_input_mouse_event))
{
	ptr_hooked_shadow_input_mouse_event = ptr;
}
void hook_shadow_input_extended_mouse_event(int (*ptr)(rdpInput*, UINT16, UINT16, UINT16,
                                                       ptr_shadow_input_extended_mouse_event))
{
	ptr_hooked_shadow_input_extended_mouse_event = ptr;
}