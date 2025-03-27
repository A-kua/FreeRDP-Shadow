#include <windows.h>
#include <freerdp/server/shadow.h>
#ifndef AKUA_CONFIG_H
#define AKUA_CONFIG_H

struct _fileTransferHead
{
	UINT16 fileNameLength;
	UINT16 filePathLength;
	BYTE* fileName;
	BYTE* filePath;
};
typedef struct _fileTransferHead fileTransferHead;

static BOOL mEnableControl = TRUE;

#define STREAM_FILE 0x05
#define STREAM_CONTROL 0x06

#define DATA_PDU_TYPE_UNKNOWN 0x40

void akua_file_process(wStream* s, BYTE* type);
void akua_control_process(wStream* s, BYTE* type);
UINT32 calcuate_file_crc32(HANDLE hFile);
#endif
