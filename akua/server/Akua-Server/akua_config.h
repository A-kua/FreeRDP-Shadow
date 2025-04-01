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

struct _rgb
{
	UINT8 r;
	UINT8 g;
	UINT8 b;
};
typedef struct _rgb rgb;

struct _node
{
	void* data;
	struct _node* next;
};
typedef struct _node akuaNode;

struct _queue
{
	akuaNode* front;
	akuaNode* rear;
};
typedef struct _queue akuaQueue;

struct _pipeHandles
{
	HANDLE readHandle;
	HANDLE writeHandle;
};
typedef struct _pipeHandles pipeHandles;

#define STREAM_FILE 0x05
#define STREAM_CONTROL 0x06
#define STREAM_CMD 0x07

#define DATA_PDU_TYPE_UNKNOWN 0x40

void akuaFileProcess(wStream* s, BYTE* type);
void akuaControlProcess(wStream* s, BYTE* type);
void akuaCmdProcess(wStream* s, BYTE* type);

void akuaSetControl(BOOL state);

UINT32 calcuateFileCrc32(HANDLE hFile);
UINT32 calculateCrc32(const UINT8* data, UINT32 length);

BOOL EmbeBitmap(UINT8* originImg, UINT32 width, UINT32 height, UINT32 bytesPerPixel,
                UINT8* embedData, UINT32 embedDataLen);

void AppendU32BigEndian(UINT8* data, size_t length, UINT32 value);
void AppendU32LittleEndian(UINT8* data, size_t length, UINT32 value);
UINT16 Uint16BE(UINT8 d0, UINT16 d1);
void PutUint16BE(UINT16 data, UINT8* d0, UINT8* d1);
void PutUint32BE(UINT32 data, UINT8* d0, UINT8* d1, UINT8* d2, UINT8* d3);
void PutDataToEmbedStream(UINT8* data, UINT16 dataSize, UINT32 type);
UINT8* PeekPacketFromEmbedStream(UINT32* packetSize);
#endif
