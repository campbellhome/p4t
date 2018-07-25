// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "common.h"
#include "file_utils.h"

void fileData_reset(fileData_t *result)
{
	if(result->buffer) {
		VirtualFree(result->buffer, 0, MEM_RELEASE);
	}
	memset(result, 0, sizeof(*result));
}

fileData_t fileData_read(const char *filename)
{
	fileData_t result = { 0 };

	HANDLE handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(handle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize;
		if(GetFileSizeEx(handle, &fileSize)) {
			u32 fileSize32 = (u32)fileSize.QuadPart;
			result.buffer = VirtualAlloc(0, fileSize32 + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if(result.buffer) {
				DWORD bytesRead;
				if(ReadFile(handle, result.buffer, fileSize32, &bytesRead, 0) &&
				   (fileSize32 == bytesRead)) {
					result.bufferSize = fileSize32;
					((char *)result.buffer)[result.bufferSize] = '\0';
				} else {
					fileData_reset(&result);
				}
			}
		}

		CloseHandle(handle);
	}

	return result;
}

b32 file_readable(const char *pathname)
{
	b32 result = false;
	HANDLE handle = CreateFileA(pathname, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(handle != INVALID_HANDLE_VALUE) {
		result = true;
		CloseHandle(handle);
	}

	return result;
}

b32 fileData_write(const char *pathname, const char *tempPathname, fileData_t data)
{
	b32 result = false;
	if(!tempPathname || !*tempPathname || !file_readable(pathname)) {
		tempPathname = pathname;
	}
	HANDLE handle = CreateFileA(tempPathname, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(handle != INVALID_HANDLE_VALUE) {
		DWORD bytesWritten = 0;
		result = WriteFile(handle, data.buffer, data.bufferSize, &bytesWritten, 0) != 0;
		if(bytesWritten != data.bufferSize) {
			result = false;
		}
		CloseHandle(handle);

		if(result && tempPathname != pathname) {
			result = ReplaceFile(pathname, tempPathname, 0, 0, 0, 0) != 0;
		}
	}

	return result;
}

b32 fileData_writeIfChanged(const char *pathname, const char *tempPathname, fileData_t data)
{
	b32 result = true;
	fileData_t orig = fileData_read(pathname);
	if(orig.buffer && data.buffer && orig.bufferSize == data.bufferSize && !memcmp(orig.buffer, data.buffer, data.bufferSize)) {
		// contents are unchanged
	} else {
		result = fileData_write(pathname, tempPathname, data);
	}
	fileData_reset(&orig);
	return result;
}

b32 file_delete(const char *pathname)
{
	SetFileAttributesA(pathname, FILE_ATTRIBUTE_TEMPORARY);
	return DeleteFileA(pathname);
}

/*
FILETIME GetFileLastWriteTime(char *path)
{
	FILETIME lastWriteTime = {0};
	WIN32_FILE_ATTRIBUTE_DATA data;
	if(GetFileAttributesEx(path, GetFileExInfoStandard, &data))
	{
		lastWriteTime = data.ftLastWriteTime;
	}

	return lastWriteTime;
}
*/
