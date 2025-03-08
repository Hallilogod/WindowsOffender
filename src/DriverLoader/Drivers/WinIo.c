#include "../DriverLoader.h"

UINT64 WinIoMapPhysicalMemory(HANDLE driverHandle, PWINIO_PHYSICAL_STRUCT pPhysicalStruct)
{
	DWORD returnValue;
	if (!NtWrapperDeviceIoControl(driverHandle,
								  IOCTL_WINIO_MAP,
								  pPhysicalStruct,
								  sizeof(WINIO_PHYSICAL_STRUCT),
								  pPhysicalStruct,
								  sizeof(WINIO_PHYSICAL_STRUCT),
								  &returnValue,
								  NULL))
	{
		return 0ULL;
	}

	return pPhysicalStruct->physicalLinear;
}

BOOL WinIoUnmapPhysicalMemory(HANDLE driverHandle, PWINIO_PHYSICAL_STRUCT pPhysicalStruct)
{
	DWORD returnValue;
	if (!NtWrapperDeviceIoControl(driverHandle,
								  IOCTL_WINIO_UNMAP,
								  pPhysicalStruct,
								  sizeof(WINIO_PHYSICAL_STRUCT),
								  NULL,
								  0,
								  &returnValue,
								  NULL))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WinIoReadPhysicalMemory(HANDLE driverHandle, PVOID startAddress, _Out_ PVOID buffer, size_t sizeInBytes)
{
	WINIO_PHYSICAL_STRUCT physicalStruct;
	physicalStruct.physicalAddress = (UINT64)startAddress;
	physicalStruct.sizeInBytes = sizeInBytes;

	UINT64 mappedMemory = WinIoMapPhysicalMemory(driverHandle, &physicalStruct);

	if (mappedMemory == 0)
	{
		return FALSE;
	}

	CopyMem((PVOID)buffer, (PVOID)mappedMemory, sizeInBytes);

	WinIoUnmapPhysicalMemory(driverHandle, &physicalStruct);

	return TRUE;
}

BOOL WinIoWritePhysicalMemory(HANDLE driverHandle, PVOID startAddress, _In_ PVOID buffer, size_t sizeInBytes)
{
	WINIO_PHYSICAL_STRUCT physStruct;
	physStruct.physicalAddress = (UINT64)startAddress;
	physStruct.sizeInBytes = sizeInBytes;

	UINT64 mappedMemory = WinIoMapPhysicalMemory(driverHandle, &physStruct);

	if (mappedMemory == 0)
	{
		return FALSE;
	}

	CopyMem((PVOID)mappedMemory, buffer, sizeInBytes);

	WinIoUnmapPhysicalMemory(driverHandle, &physStruct);

	return TRUE;
}