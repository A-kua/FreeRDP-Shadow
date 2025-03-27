#include "akua_config.h"

BOOL recv_file_transfer_start(fileTransferHead* head, wStream* s)
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

BOOL recv_file_transfer_abort(fileTransferHead* head, wStream* s)
{
	char fullPath[MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s\\%s", head->filePath, head->fileName);

	if (GetFileAttributesA(fullPath) != INVALID_FILE_ATTRIBUTES)
		if (!DeleteFileA(fullPath))
			return FALSE;

	return TRUE;
}

BOOL recv_file_transfer_packet(fileTransferHead* head, wStream* s)
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
	//printf("StartIndex %d %256s\n", startIndex, fileDataSlicing);
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

BOOL recv_file_transfer_verify(fileTransferHead* head, wStream* s)
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

	realCRC32 = calcuate_file_crc32(hFile);

	printf("crc32 %d %d\n", targetCRC32, realCRC32);

	CloseHandle(hFile);
	return bak;
}

void akua_file_process(wStream* s, BYTE* type)
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
	//printf("Type %hhu: %s %s %d %d\n", *type, head->fileName, head->filePath, head->fileNameLength,
	//       head->filePathLength);

	switch (*type)
	{
		case 0x36:
		{
			if (!recv_file_transfer_start(head, s))
				goto seek_remaining;
			break;
		}
		case 0x27:
		{
			if (!recv_file_transfer_abort(head, s))
				goto seek_remaining;
			break;
		}
		case 0x21:
		{
			if (!recv_file_transfer_packet(head, s))
				goto seek_remaining;
			break;
		}
		case 0x02:
		{
			if (!recv_file_transfer_verify(head, s))
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