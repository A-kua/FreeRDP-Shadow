#include "akua_config.h"

void sendFileTransferVerifyState(fileTransferHead* head, BOOL state)
{
	UINT32 dataLen =
	    sizeof(UINT8) + 2 * sizeof(UINT16) + head->fileNameLength + head->filePathLength;
	UINT8* data = (UINT8*)malloc(dataLen);
	PutUint16BE(head->fileNameLength, &data[1], &data[0]);
	PutUint16BE(head->filePathLength, &data[3], &data[2]);
	memcpy(data + 4, head->fileName, head->fileNameLength);
	memcpy(data + 4 + head->fileNameLength, head->filePath, head->filePathLength);
	if (state)
	{
		data[4 + head->fileNameLength + head->filePathLength] = 0xff;
	}
	else
	{
		data[4 + head->fileNameLength + head->filePathLength] = 0x00;
	}
	PutDataToEmbedStream(data, dataLen, 0x02);
}

BOOL recvFileTransferStart(fileTransferHead* head, wStream* s)
{
	BOOL bak = TRUE;
	char fullPath[MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s\\%s", head->filePath, head->fileName);

	if (GetFileAttributesA(fullPath) != INVALID_FILE_ATTRIBUTES)
		if (!DeleteFileA(fullPath))
			return FALSE;

	HANDLE hFile =
	    CreateFileA(fullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	UINT32 fileSize;
	Stream_Read_UINT32(s, fileSize);

	DWORD newPosition = SetFilePointer(hFile, fileSize - 1, NULL, FILE_BEGIN);

	if (newPosition == INVALID_SET_FILE_POINTER)
	{
		bak = FALSE;
		goto ret;
	}

	if (!SetEndOfFile(hFile))
	{
		bak = FALSE;
		goto ret;
	}
ret:
	CloseHandle(hFile);
	return TRUE;
}

BOOL recvFileTransferAbort(fileTransferHead* head, wStream* s)
{
	char fullPath[MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s\\%s", head->filePath, head->fileName);

	if (GetFileAttributesA(fullPath) != INVALID_FILE_ATTRIBUTES)
		if (!DeleteFileA(fullPath))
			return FALSE;

	return TRUE;
}

BOOL recvFileTransferPacket(fileTransferHead* head, wStream* s)
{
	BOOL bak = TRUE;
	char fullPath[MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s\\%s", head->filePath, head->fileName);

	if (GetFileAttributesA(fullPath) == INVALID_FILE_ATTRIBUTES)
		return FALSE;
	HANDLE hFile =
	    CreateFileA(fullPath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	UINT32 startIndex;
	UINT32 slicingSize;
	BYTE* fileDataSlicing;
	Stream_Read_UINT32(s, startIndex);
	Stream_Read_UINT32(s, slicingSize);
	fileDataSlicing = (BYTE*)calloc(1, slicingSize);
	Stream_Read(s, fileDataSlicing, slicingSize);
	// printf("StartIndex %d %256s\n", startIndex, fileDataSlicing);
	if (SetFilePointer(hFile, startIndex, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		bak = FALSE;
		goto ret;
	}

	DWORD bytesWritten;
	if (!WriteFile(hFile, fileDataSlicing, slicingSize, &bytesWritten, NULL))
	{
		bak = FALSE;
		goto ret;
	}
	else if (bytesWritten != slicingSize)
	{
		bak = FALSE;
		goto ret;
	}
ret:
	CloseHandle(hFile);
	free(fileDataSlicing);
	return bak;
}

BOOL recvFileTransferVerify(fileTransferHead* head, wStream* s)
{
	BOOL bak = TRUE;
	char fullPath[MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s\\%s", head->filePath, head->fileName);

	if (GetFileAttributesA(fullPath) == INVALID_FILE_ATTRIBUTES)
		return FALSE;

	UINT32 fileSize;
	UINT32 targetCRC32, realCRC32;
	Stream_Read_UINT32(s, fileSize);
	Stream_Read_UINT32(s, targetCRC32);

	HANDLE hFile = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
	                           FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	realCRC32 = calcuateFileCrc32(hFile);

	sendFileTransferVerifyState(head, (BOOL)targetCRC32 == realCRC32);

	CloseHandle(hFile);
	return bak;
}

void akuaFileProcess(wStream* s, BYTE* type)
{
	fileTransferHead* head = (fileTransferHead*)calloc(1, sizeof(fileTransferHead));
	if (!head)
		return;
	Stream_Read_UINT16(s, head->fileNameLength);
	Stream_Read_UINT16(s, head->filePathLength);

	head->fileName = (BYTE*)calloc(1, head->fileNameLength);
	head->filePath = (BYTE*)calloc(1, head->filePathLength);
	Stream_Read(s, head->fileName, head->fileNameLength);
	Stream_Read(s, head->filePath, head->filePathLength);
	// printf("Type %hhu: %s %s %d %d\n", *type, head->fileName, head->filePath,
	// head->fileNameLength,
	//        head->filePathLength);

	switch (*type)
	{
		case 0x36:
		{
			if (!recvFileTransferStart(head, s))
				goto seek_remaining;
			break;
		}
		case 0x27:
		{
			if (!recvFileTransferAbort(head, s))
				goto seek_remaining;
			break;
		}
		case 0x21:
		{
			if (!recvFileTransferPacket(head, s))
				goto seek_remaining;
			break;
		}
		case 0x02:
		{
			if (!recvFileTransferVerify(head, s))
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
	free(head->fileName);
	free(head->filePath);
	free(head);
}