#pragma once

BOOL NtWrapperDeviceIoControl(HANDLE hDevice,
							  DWORD dwIoControlCode,
							  LPVOID lpInBuffer,
							  DWORD nInBufferSize,
							  LPVOID lpOutBuffer,
							  DWORD nOutBufferSize,
							  LPDWORD lpBytesReturned,
							  LPOVERLAPPED lpOverlapped);

PVOID CopyMem(PVOID destination, PVOID source, size_t bytesToCopy);

PVOID FindKernelBasePhysical(HANDLE driverHandle, ReadMemoryDriverRoutine readMemoryDriverRoutine);

PVOID SearchForPatternPhysical(
	HANDLE driverHandle,
	ReadMemoryDriverRoutine physicalReadMemoryDriverRoutine,
	PVOID startAddress,
	size_t searchRange,
	PBYTE pattern,
	size_t sizeOfPattern);

BOOL FindModuleResource(
	HMODULE module,
	PWSTR resourceDirectoryName,
	PWSTR resourceName,
	_Out_ PBIN_RESOURCE pBinResource);

NTSTATUS WriteBinResourceToFile(
	PBIN_RESOURCE pBinResource,
	PCWSTR fullFilePath,
	_Out_opt_ PHANDLE pOutputFileHandle);

NTSTATUS AddVolatileDriverService(LPWSTR driverName, LPWSTR driverImagePath, _Out_opt_ PHANDLE pOutputKeyHandle);

NTSTATUS DeleteFileByDeleteOnClose(LPWSTR fullFilePath);

NTSTATUS DeleteFileByName(LPWSTR fullFilePath);