#include "Main.h"

// Used to send the information to the driver via DeviceIoControl
SERVER_STATUS_CODE SendIOCTL(
	DWORD ioctlCode,
	PVOID ioctlInputBuffer,
	DWORD ioctlInputBufferSize,
	_Out_ PVOID outputBuffer,
	DWORD outputBufferSize,
	_Out_ LPDWORD bytesReturned)
{

	HANDLE driverHandle = CreateFileA("\\\\.\\" DRIVER_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (driverHandle == INVALID_HANDLE_VALUE)
	{
		dbgprintf("Failed to get a handle to the driver (\\\\.\\" DRIVER_NAME "), lasterror: %ld\n", GetLastError());

		return SERVER_STATUS_DRIVER_IOCTL_ERROR;
	}

	// Here we send the message to the driver, 
	if (!DeviceIoControl(driverHandle, ioctlCode, ioctlInputBuffer, ioctlInputBufferSize, outputBuffer, outputBufferSize, bytesReturned, NULL))
	{

		dbgprintf("DeviceIoControl failed! Lasterror: %ld\n", GetLastError());

		CloseHandle(driverHandle);

		return SERVER_STATUS_DRIVER_IOCTL_ERROR;
	}

	CloseHandle(driverHandle);

	return SERVER_STATUS_SUCCESS;
}