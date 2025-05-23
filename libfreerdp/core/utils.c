/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Terminal Server Gateway (utils)
 *
 * Copyright 2021 Armin Novak <armin.novak@thincast.com>
 * Copyright 2021 Thincast Technologies GmbH
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

#include <winpr/assert.h>

#include <freerdp/freerdp.h>

#include <freerdp/log.h>
#define TAG FREERDP_TAG("core.gateway.utils")

#include "utils.h"

BOOL utils_str_copy(const char* value, char** dst)
{
	WINPR_ASSERT(dst);

	free(*dst);
	*dst = NULL;
	if (!value)
		return TRUE;

	(*dst) = _strdup(value);
	return (*dst) != NULL;
}

BOOL utils_abort_connect(rdpContext* context)
{
	WINPR_ASSERT(context);
	if (context->abortEvent)
		return SetEvent(context->abortEvent);
	return FALSE;
}

BOOL utils_reset_abort(rdpContext* context)
{
	WINPR_ASSERT(context);
	return ResetEvent(context->abortEvent);
}

BOOL utils_str_is_empty(const char* str)
{
	if (!str)
		return TRUE;
	if (strlen(str) == 0)
		return TRUE;
	return FALSE;
}
