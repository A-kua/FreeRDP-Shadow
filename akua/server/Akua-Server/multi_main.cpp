#include <windows.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/crt.h>
#include <winpr/ssl.h>
#include <winpr/wnd.h>
#include <winpr/path.h>
#include <winpr/cmdline.h>
#include <winpr/winsock.h>

#include <winpr/tools/makecert.h>

#include <freerdp/server/shadow.h>

#include <freerdp/codec/bitmap.h>

static void akua_main();

extern void initBitmapStream();
extern void destroyBitmapStream();
extern SSIZE_T akua_freerdp_bitmap_compress(const void*, UINT32, UINT32, wStream*, UINT32, UINT32,
                                            UINT32, wStream*, UINT32, ptr_freerdp_bitmap_compress);
extern BOOL akua_rdp_read_share_data_header(wStream*, UINT16*, BYTE*, UINT32*, BYTE*, UINT16*,
                                            ptr_rdp_read_share_data_header);
extern BOOL akua_shadow_input_synchronize_event(rdpInput*, UINT32,
                                                ptr_shadow_input_synchronize_event);
extern BOOL akua_shadow_input_keyboard_event(rdpInput*, UINT16, UINT8,
                                             ptr_shadow_input_keyboard_event);
extern BOOL akua_shadow_input_unicode_keyboard_event(rdpInput*, UINT16, UINT16,
                                                     ptr_shadow_input_unicode_keyboard_event);
extern BOOL akua_shadow_input_mouse_event(rdpInput*, UINT16, UINT16, UINT16,
                                          ptr_shadow_input_mouse_event);
extern BOOL akua_shadow_input_extended_mouse_event(rdpInput*, UINT16, UINT16, UINT16,
                                                   ptr_shadow_input_extended_mouse_event);
void setup_server(rdpShadowServer*);

int main()
{
	hook_freerdp_bitmap_compress(akua_freerdp_bitmap_compress);
	hook_rdp_read_share_data_header(akua_rdp_read_share_data_header);
	hook_shadow_input_synchronize_event(akua_shadow_input_synchronize_event);
	hook_shadow_input_keyboard_event(akua_shadow_input_keyboard_event);
	hook_shadow_input_unicode_keyboard_event(akua_shadow_input_unicode_keyboard_event);
	hook_shadow_input_mouse_event(akua_shadow_input_mouse_event);
	hook_shadow_input_extended_mouse_event(akua_shadow_input_extended_mouse_event);
	initBitmapStream();
	akua_main();
	destroyBitmapStream();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd = GetConsoleWindow();
	if (hWnd != NULL)
	{
		ShowWindow(hWnd, SW_HIDE);
	}
	main();
	return 0;
}

void akua_main()
{
	MSG msg;
	int status = 0;
	DWORD dwExitCode;

	rdpShadowServer* server = (rdpShadowServer*)calloc(1, sizeof(rdpShadowServer));

	if (!server)
		return;

	shadow_subsystem_set_entry_builtin(NULL);

	setup_server(server);

	if ((status = shadow_server_init(server)) < 0)
		goto fail_server_init;

	if ((status = shadow_server_start(server)) < 0)
		goto fail_server_start;

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WaitForSingleObject(server->thread, INFINITE);

	GetExitCodeThread(server->thread, &dwExitCode);

fail_server_start:
	shadow_server_uninit(server);
fail_server_init:
	shadow_server_free(server);
}

void setup_server(rdpShadowServer* server)
{
	rdpSettings* settings;

	server->port = 3388;
	server->selectedMonitor = 0;
	server->mayView = TRUE;
	server->mayInteract = TRUE;
	server->rfxMode = RLGR3;
	server->h264RateControlMode = H264_RATECONTROL_VBR;
	server->h264BitRate = 100000;
	server->h264FrameRate = 60;
	server->h264QP = 0;
	server->authentication = FALSE;
	size_t len = strlen("0.0.0.0");
	server->ipcSocket = (char*)calloc(len, sizeof(CHAR));
	_snprintf(server->ipcSocket, len, "%s", "0.0.0.0");

	settings = freerdp_settings_new(FREERDP_SETTINGS_SERVER_MODE);

	settings->NlaSecurity = FALSE;
	settings->TlsSecurity = TRUE;
	settings->RdpSecurity = TRUE;

	server->settings = settings;
}