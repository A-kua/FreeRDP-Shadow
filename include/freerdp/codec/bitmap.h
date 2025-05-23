/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Compressed Bitmap
 *
 * Copyright 2011 Jay Sorg <jay.sorg@gmail.com>
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

#ifndef FREERDP_CODEC_BITMAP_H
#define FREERDP_CODEC_BITMAP_H

#include <freerdp/api.h>
#include <freerdp/types.h>

#include <freerdp/codec/color.h>

#include <winpr/crt.h>
#include <winpr/stream.h>

#ifdef __cplusplus
extern "C"
{
#endif

	FREERDP_API SSIZE_T freerdp_bitmap_compress(const void* in_data, UINT32 width, UINT32 height,
	                                            wStream* s, UINT32 bpp, UINT32 byte_limit,
	                                            UINT32 start_line, wStream* temp_s, UINT32 e);
	typedef SSIZE_T (*ptr_freerdp_bitmap_compress)(const void*, UINT32, UINT32, wStream*, UINT32,
	                                               UINT32, UINT32, wStream*, UINT32);
	FREERDP_API void hook_freerdp_bitmap_compress(
	    SSIZE_T (*ptr)(const void*, UINT32, UINT32, wStream*, UINT32, UINT32, UINT32, wStream*,
	                   UINT32, ptr_freerdp_bitmap_compress origin));
#ifdef __cplusplus
}
#endif

#endif /* FREERDP_CODEC_BITMAP_H */
