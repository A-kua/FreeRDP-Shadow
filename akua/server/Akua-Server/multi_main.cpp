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

static void akua_main();
extern BOOL akua_shadow_input_mouse_event(rdpInput*, UINT16, UINT16, UINT16,
                                          ptr_shadow_input_mouse_event);
extern void setup_server(rdpShadowServer*);

int main()
{
	hook_shadow_input_mouse_event(akua_shadow_input_mouse_event);
	akua_main();
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
