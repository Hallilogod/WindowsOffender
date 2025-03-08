#pragma once

#define IOCTL_WINIO_MAP 0x80102040
#define IOCTL_WINIO_UNMAP 0x80102044


typedef struct _WINIO_PHYSICAL_STRUCT
{
	UINT64 sizeInBytes;
	UINT64 physicalAddress;
	UINT64 physicalHandle;
	UINT64 physicalLinear;
	UINT64 physicalSection;
	
} WINIO_PHYSICAL_STRUCT, *PWINIO_PHYSICAL_STRUCT;



BOOL WinIoReadPhysicalMemory(HANDLE driverHandle, PVOID startAddress, _Out_ PVOID buffer, size_t sizeInBytes);

BOOL WinIoWritePhysicalMemory(HANDLE driverHandle, PVOID startAddress, _In_ PVOID buffer, size_t sizeInBytes);
