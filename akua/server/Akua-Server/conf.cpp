#include <freerdp/server/shadow.h>

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