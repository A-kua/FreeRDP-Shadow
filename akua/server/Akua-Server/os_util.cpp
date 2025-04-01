#include "akua_config.h"

pipeHandles* createPipeWithMalloc()
{
	pipeHandles* pipes = (pipeHandles*)malloc(sizeof(pipeHandles));
	if (pipes == NULL)
	{
		return NULL;
	}

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&pipes->readHandle, &pipes->writeHandle, &saAttr, 0))
	{
		free(pipes);
		return NULL;
	}

	return pipes;
}

PROCESS_INFORMATION* createCommandline(HANDLE in, HANDLE out)
{
	STARTUPINFO si;
	PROCESS_INFORMATION* pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = out;
	si.hStdOutput = out;
	si.hStdInput = in;
	// si.dwFlags = STARTF_USESHOWWINDOW;
	si.dwFlags |= STARTF_USESTDHANDLES;
	// si.wShowWindow = FALSE;

	pi = (PROCESS_INFORMATION*)malloc(sizeof(PROCESS_INFORMATION));
	if (pi == NULL)
		return NULL;

	char command[] = "C:\\Windows\\System32\\cmd.exe";
	wchar_t cmd[2048] = { 0 };
	size_t converted;
	mbstowcs_s(&converted, cmd, sizeof(cmd) / sizeof(wchar_t), command, _TRUNCATE);

	if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, pi))
	{
		return NULL;
	}
	return pi;
}