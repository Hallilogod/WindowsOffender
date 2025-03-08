#include "DriverLoader.h"

extern PVOID g_ProcessHeap;

BOOL NtWrapperDeviceIoControl(HANDLE hDevice,
							  DWORD dwIoControlCode,
							  LPVOID lpInBuffer,
							  DWORD nInBufferSize,
							  LPVOID lpOutBuffer,
							  DWORD nOutBufferSize,
							  LPDWORD lpBytesReturned,
							  LPOVERLAPPED lpOverlapped)
{

	IO_STATUS_BLOCK ioStatusBlock;

	NTSTATUS status = NtDeviceIoControlFile(hDevice,
											NULL,
											NULL,
											NULL,
											&ioStatusBlock,
											dwIoControlCode,
											lpInBuffer,
											nInBufferSize,
											lpOutBuffer,
											nOutBufferSize);

	return NT_SUCCESS(status) ? TRUE : FALSE;
}

PVOID CopyMem(PVOID destination, PVOID source, size_t bytesToCopy)
{

	PBYTE destinationBytePointer = (PBYTE)destination;
	PBYTE sourceBytePointer = (PBYTE)source;

	while (bytesToCopy--)
	{
		*destinationBytePointer++ = *sourceBytePointer++;
	}

	return destination;
}

size_t WcsLen(LPCWSTR string)
{
	size_t i = 0;

	while (string[i] != L'\0')
	{
		++i;
	}

	return i;
}

PVOID SearchForPatternPhysical(
	HANDLE driverHandle,
	ReadMemoryDriverRoutine physicalReadMemoryDriverRoutine,
	PVOID startAddress,
	size_t searchRange,
	PBYTE pattern,
	size_t sizeOfPattern)
{

	PVOID result = NULL;
	PBYTE buffer = RtlAllocateHeap(g_ProcessHeap, HEAP_NO_SERIALIZE, searchRange);

	if(buffer == NULL)
	{
		return NULL;
	}

	if(!physicalReadMemoryDriverRoutine(driverHandle, startAddress, buffer, searchRange))
	{
		result = NULL;
		goto _SearchForPatternPhysical_Exit;
	}

	size_t maxOffset = (searchRange >= sizeOfPattern) ? (searchRange - sizeOfPattern + 1) : 0;

    for (size_t i = 0; i < maxOffset; i++)
	{
        size_t j;
        for (j = 0; j < sizeOfPattern; j++)
		{
            if (pattern[j] != 0x00 && buffer[i + j] != pattern[j])
                break;
        }
        if (j == sizeOfPattern)	// Check for full match
		{  
            result = (PVOID)((PBYTE)startAddress + i);
            break;
        }
    }

_SearchForPatternPhysical_Exit:
	RtlFreeHeap(g_ProcessHeap, HEAP_NO_SERIALIZE, buffer);
	return result;
}

PVOID FindKernelBasePhysical(HANDLE driverHandle, ReadMemoryDriverRoutine physicalReadMemoryDriverRoutine)
{
	UINT64 kernelBase = 0;
	char buffer[2] = { 0 };
	for (size_t i = 0; i < 0x200000000; i += 0x100000)
	{
		physicalReadMemoryDriverRoutine(driverHandle, (PVOID)i, buffer, 2);

		if (buffer[0] == 'M' && buffer[1] == 'Z')
		{
			return (PVOID)i;
		}
	}

	return NULL;
}

// This took 10 HOURS~ TO MAKE
BOOL FindModuleResource(HMODULE module, PWSTR resourceDirectoryName, PWSTR resourceName, _Out_ PBIN_RESOURCE pBinResource)
{
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)module + dosHeader->e_lfanew);

	LDR_RESOURCE_INFO resourceInfo = {0};
	PIMAGE_RESOURCE_DATA_ENTRY resourceDataEntry;

	resourceInfo.Name = (ULONG_PTR)resourceName;
	resourceInfo.Type = (ULONG_PTR)resourceDirectoryName;

	NTSTATUS status = LdrFindResource_U(module, &resourceInfo, 3U, &resourceDataEntry);

	if (!NT_SUCCESS(status))
	{
		dbgprintf("Error finding resource %ws\n", resourceName);

		return FALSE;
	}

	pBinResource->BufferSizeBytes = resourceDataEntry->Size;
	pBinResource->DataBuffer = (PVOID)(ntHeader->OptionalHeader.ImageBase + resourceDataEntry->OffsetToData);

	return TRUE;
}

NTSTATUS WriteBinResourceToFile(PBIN_RESOURCE pBinResource, PCWSTR fullFilePath, _Out_opt_ PHANDLE pOutputFileHandle)
{
	NTSTATUS status;
	HANDLE fileHandle = NULL;
	IO_STATUS_BLOCK ioStatusBlock = {0};
	UNICODE_STRING fullFilePathUnicodeStr = {0};
	OBJECT_ATTRIBUTES fileObjectAttributes = {0};
	LARGE_INTEGER fileByteOffset = {0};

	RtlInitUnicodeString(&fullFilePathUnicodeStr, fullFilePath);

	InitializeObjectAttributes(&fileObjectAttributes, &fullFilePathUnicodeStr, OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = NtCreateFile(
		&fileHandle,
		FILE_READ_DATA | FILE_WRITE_DATA,
		&fileObjectAttributes,
		&ioStatusBlock,
		0,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_CREATE,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = NtWriteFile(
		fileHandle,
		NULL,
		NULL,
		NULL,
		&ioStatusBlock,
		pBinResource->DataBuffer,
		pBinResource->BufferSizeBytes,
		&fileByteOffset,
		NULL);

	if (!NT_SUCCESS(status))
	{
		NtClose(fileHandle);

		NtDeleteFile(&fileObjectAttributes);

		return status;
	}

	if (pOutputFileHandle == NULL)
	{
		NtClose(fileHandle);
	}
	else
	{
		*pOutputFileHandle = fileHandle;
	}

	return status;
}

NTSTATUS DeleteFileByName(LPWSTR fullFilePath)
{
	UNICODE_STRING fullFilePathUnicodeStr = {0};
	OBJECT_ATTRIBUTES fileObjectAttributes = {0};

	RtlInitUnicodeString(&fullFilePathUnicodeStr, fullFilePath);

	InitializeObjectAttributes(&fileObjectAttributes, &fullFilePathUnicodeStr, OBJ_CASE_INSENSITIVE, NULL, NULL);

	return NtDeleteFile(&fileObjectAttributes);
}


NTSTATUS AddVolatileDriverService(LPWSTR driverRegistryPath, LPWSTR driverImagePath, _Out_opt_ PHANDLE pOutputKeyHandle)
{
	UNICODE_STRING driverRegistryKeyName = {0};
	UNICODE_STRING imagePathKeyValueName = {0};
	UNICODE_STRING typeKeyValueName = {0};
	UNICODE_STRING startKeyValueName = {0};
	UNICODE_STRING errorControlKeyValueName = {0};
	DWORD driverServiceType = SERVICE_KERNEL_DRIVER;
	DWORD driverStartType = SERVICE_DEMAND_START;
	DWORD driverErrorControlType = 1;
	OBJECT_ATTRIBUTES driverKeyObjectAttributes = {0};
	HANDLE driverKeyHandle = NULL;
	ULONG driverKeyDisposition;
	NTSTATUS status;

	RtlInitUnicodeString(&driverRegistryKeyName, driverRegistryPath);
	RtlInitUnicodeString(&imagePathKeyValueName, L"ImagePath");
	RtlInitUnicodeString(&typeKeyValueName, L"Type");
	RtlInitUnicodeString(&startKeyValueName, L"Start");
	RtlInitUnicodeString(&errorControlKeyValueName, L"ErrorControl");

	InitializeObjectAttributes(&driverKeyObjectAttributes, &driverRegistryKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = NtCreateKey(
		&driverKeyHandle,
		KEY_READ | KEY_WRITE | DELETE,
		&driverKeyObjectAttributes,
		0,
		NULL,
		REG_OPTION_VOLATILE,
		&driverKeyDisposition);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (REG_CREATED_NEW_KEY != driverKeyDisposition)
	{
		NtClose(driverKeyHandle);

		return STATUS_ALREADY_REGISTERED;
	}

	status = NtSetValueKey(
		driverKeyHandle,
		&imagePathKeyValueName,
		0,
		REG_EXPAND_SZ,
		driverImagePath,
		(WcsLen(driverImagePath) + 1) * sizeof(WCHAR));

	if (!NT_SUCCESS(status))
	{
		NtDeleteKey(driverKeyHandle);
		NtClose(driverKeyHandle);

		return status;
	}

	status = NtSetValueKey(
		driverKeyHandle,
		&typeKeyValueName,
		0,
		REG_DWORD,
		&driverServiceType,
		sizeof(DWORD));

	if (!NT_SUCCESS(status))
	{
		NtDeleteKey(driverKeyHandle);
		NtClose(driverKeyHandle);

		return status;
	}

	status = NtSetValueKey(
		driverKeyHandle,
		&startKeyValueName,
		0,
		REG_DWORD,
		&driverStartType,
		sizeof(DWORD));

	if (!NT_SUCCESS(status))
	{
		NtDeleteKey(driverKeyHandle);
		NtClose(driverKeyHandle);

		return status;
	}

	status = NtSetValueKey(
		driverKeyHandle,
		&errorControlKeyValueName,
		0,
		REG_DWORD,
		&driverErrorControlType,
		sizeof(DWORD));

	if (!NT_SUCCESS(status))
	{
		NtDeleteKey(driverKeyHandle);
		NtClose(driverKeyHandle);

		return status;
	}

	if (pOutputKeyHandle == NULL)
	{
		NtClose(driverKeyHandle);
	}
	else
	{
		*pOutputKeyHandle = driverKeyHandle;
	}

	return status;
}


