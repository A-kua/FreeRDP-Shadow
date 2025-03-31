#include "akua_config.h"

extern akuaQueue* createQueue();
extern void enqueue(akuaQueue* queue, void* data);
extern void* dequeue(akuaQueue* queue);
extern void freeQueue(akuaQueue* queue);

akuaQueue* embedDataQueue;

void initBitmapStream()
{
	embedDataQueue = createQueue();
}

void destroyBitmapStream()
{
	freeQueue(embedDataQueue);
}
struct _packet
{
	UINT8* data;
	UINT32 packetSize;
};
typedef struct _packet akuaPacket;

void PutDataToEmbedStream(UINT8* data, UINT16 dataSize, UINT32 type)
{
	int packetSize = (int)dataSize + 2 * sizeof(UINT16) + 2 * sizeof(UINT32);
	UINT8* packetData = (UINT8*)malloc(packetSize);
	PutUint16BE(0xdead, &packetData[0], &packetData[1]);
	PutUint16BE(dataSize, &packetData[2], &packetData[3]);
	PutUint32BE(type, &packetData[4], &packetData[5], &packetData[6], &packetData[7]);
	memcpy(packetData + 8, data, dataSize);
	UINT32 crc32 = calculateCrc32(data, dataSize);
	AppendU32BigEndian(packetData, packetSize-4, crc32);
	akuaPacket* packet = (akuaPacket*)malloc(sizeof(akuaPacket));
	packet->data = packetData;
	packet->packetSize = packetSize;
	enqueue(embedDataQueue, packet);
	free(data);
}

UINT8* PeekPacketFromEmbedStream(UINT32* packetSize)
{
	akuaPacket* packet = (akuaPacket*)dequeue(embedDataQueue);
	if (NULL == packet)
	{
		return NULL;
	}
	*packetSize = packet->packetSize;
	UINT8* data = packet->data;
	free(packet);
	return data;
}
