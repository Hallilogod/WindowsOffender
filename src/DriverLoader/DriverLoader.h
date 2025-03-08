#pragma once

#include "../Shared.h"

#include <Windows.h>
#include <stdbool.h>

/*
We dont include winternl and do all the ugly shit ourselves because some definitons in there are incomplete
*/

#ifdef DEBUG
#define dbgprintf printf
#else
#define dbgprintf
#endif

#define SE_LOAD_DRIVER_PRIVILEGE (10L)

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;

	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID(NTAPI *PIO_APC_ROUTINE)(
	IN PVOID ApcContext,
	IN PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG Reserved);

typedef struct _PEB
{
	UCHAR InheritedAddressSpace;	// 0x0
	UCHAR ReadImageFileExecOptions; // 0x1
	UCHAR BeingDebugged;			// 0x2
	union
	{
		UCHAR BitField; // 0x3
		struct
		{
			UCHAR ImageUsesLargePages : 1;			// 0x3
			UCHAR IsProtectedProcess : 1;			// 0x3
			UCHAR IsImageDynamicallyRelocated : 1;	// 0x3
			UCHAR SkipPatchingUser32Forwarders : 1; // 0x3
			UCHAR IsPackagedProcess : 1;			// 0x3
			UCHAR IsAppContainer : 1;				// 0x3
			UCHAR IsProtectedProcessLight : 1;		// 0x3
			UCHAR IsLongPathAwareProcess : 1;		// 0x3
		};
	};
	UCHAR Padding0[4];										// 0x4
	VOID *Mutant;											// 0x8
	VOID *ImageBaseAddress;									// 0x10
	struct _PEB_LDR_DATA *Ldr;								// 0x18
	struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters; // 0x20
	VOID *SubSystemData;									// 0x28
	VOID *ProcessHeap;										// 0x30
	struct _RTL_CRITICAL_SECTION *FastPebLock;				// 0x38
	union _SLIST_HEADER *volatile AtlThunkSListPtr;			// 0x40
	VOID *IFEOKey;											// 0x48
	union
	{
		ULONG CrossProcessFlags; // 0x50
		struct
		{
			ULONG ProcessInJob : 1;				  // 0x50
			ULONG ProcessInitializing : 1;		  // 0x50
			ULONG ProcessUsingVEH : 1;			  // 0x50
			ULONG ProcessUsingVCH : 1;			  // 0x50
			ULONG ProcessUsingFTH : 1;			  // 0x50
			ULONG ProcessPreviouslyThrottled : 1; // 0x50
			ULONG ProcessCurrentlyThrottled : 1;  // 0x50
			ULONG ProcessImagesHotPatched : 1;	  // 0x50
			ULONG ReservedBits0 : 24;			  // 0x50
		};
	};
	UCHAR Padding1[4]; // 0x54
	union
	{
		VOID *KernelCallbackTable; // 0x58
		VOID *UserSharedInfoPtr;   // 0x58
	};
	ULONG SystemReserved;						 // 0x60
	ULONG AtlThunkSListPtr32;					 // 0x64
	VOID *ApiSetMap;							 // 0x68
	ULONG TlsExpansionCounter;					 // 0x70
	UCHAR Padding2[4];							 // 0x74
	VOID *TlsBitmap;							 // 0x78
	ULONG TlsBitmapBits[2];						 // 0x80
	VOID *ReadOnlySharedMemoryBase;				 // 0x88
	VOID *SharedData;							 // 0x90
	VOID **ReadOnlyStaticServerData;			 // 0x98
	VOID *AnsiCodePageData;						 // 0xa0
	VOID *OemCodePageData;						 // 0xa8
	VOID *UnicodeCaseTableData;					 // 0xb0
	ULONG NumberOfProcessors;					 // 0xb8
	ULONG NtGlobalFlag;							 // 0xbc
	union _LARGE_INTEGER CriticalSectionTimeout; // 0xc0
	ULONGLONG HeapSegmentReserve;				 // 0xc8
	ULONGLONG HeapSegmentCommit;				 // 0xd0
	ULONGLONG HeapDeCommitTotalFreeThreshold;	 // 0xd8
	ULONGLONG HeapDeCommitFreeBlockThreshold;	 // 0xe0
	ULONG NumberOfHeaps;						 // 0xe8
	ULONG MaximumNumberOfHeaps;					 // 0xec
	VOID **ProcessHeaps;						 // 0xf0
	VOID *GdiSharedHandleTable;					 // 0xf8
	VOID *ProcessStarterHelper;					 // 0x100
	ULONG GdiDCAttributeList;					 // 0x108
	UCHAR Padding3[4];							 // 0x10c
	struct _RTL_CRITICAL_SECTION *LoaderLock;	 // 0x110
	ULONG OSMajorVersion;						 // 0x118
	ULONG OSMinorVersion;						 // 0x11c
	USHORT OSBuildNumber;						 // 0x120
	USHORT OSCSDVersion;						 // 0x122
	ULONG OSPlatformId;							 // 0x124
	ULONG ImageSubsystem;						 // 0x128
	ULONG ImageSubsystemMajorVersion;			 // 0x12c
	ULONG ImageSubsystemMinorVersion;			 // 0x130
	UCHAR Padding4[4];							 // 0x134
	ULONGLONG ActiveProcessAffinityMask;		 // 0x138
	ULONG GdiHandleBuffer[60];					 // 0x140
	VOID(*PostProcessInitRoutine)
	();																	 // 0x230
	VOID *TlsExpansionBitmap;											 // 0x238
	ULONG TlsExpansionBitmapBits[32];									 // 0x240
	ULONG SessionId;													 // 0x2c0
	UCHAR Padding5[4];													 // 0x2c4
	union _ULARGE_INTEGER AppCompatFlags;								 // 0x2c8
	union _ULARGE_INTEGER AppCompatFlagsUser;							 // 0x2d0
	VOID *pShimData;													 // 0x2d8
	VOID *AppCompatInfo;												 // 0x2e0
	struct _UNICODE_STRING CSDVersion;									 // 0x2e8
	struct _ACTIVATION_CONTEXT_DATA *ActivationContextData;				 // 0x2f8
	struct _ASSEMBLY_STORAGE_MAP *ProcessAssemblyStorageMap;			 // 0x300
	struct _ACTIVATION_CONTEXT_DATA *SystemDefaultActivationContextData; // 0x308
	struct _ASSEMBLY_STORAGE_MAP *SystemAssemblyStorageMap;				 // 0x310
	ULONGLONG MinimumStackCommit;										 // 0x318
	VOID *SparePointers[4];												 // 0x320
	ULONG SpareUlongs[5];												 // 0x340
	VOID *WerRegistrationData;											 // 0x358
	VOID *WerShipAssertPtr;												 // 0x360
	VOID *pUnused;														 // 0x368
	VOID *pImageHeaderHash;												 // 0x370
	union
	{
		ULONG TracingFlags; // 0x378
		struct
		{
			ULONG HeapTracingEnabled : 1;	   // 0x378
			ULONG CritSecTracingEnabled : 1;   // 0x378
			ULONG LibLoaderTracingEnabled : 1; // 0x378
			ULONG SpareTracingBits : 29;	   // 0x378
		};
	};
	UCHAR Padding6[4];							  // 0x37c
	ULONGLONG CsrServerReadOnlySharedMemoryBase;  // 0x380
	ULONGLONG TppWorkerpListLock;				  // 0x388
	struct _LIST_ENTRY TppWorkerpList;			  // 0x390
	VOID *WaitOnAddressHashTable[128];			  // 0x3a0
	VOID *TelemetryCoverageHeader;				  // 0x7a0
	ULONG CloudFileFlags;						  // 0x7a8
	ULONG CloudFileDiagFlags;					  // 0x7ac
	CHAR PlaceholderCompatibilityMode;			  // 0x7b0
	CHAR PlaceholderCompatibilityModeReserved[7]; // 0x7b1
	struct _LEAP_SECOND_DATA *LeapSecondData;	  // 0x7b8
	union
	{
		ULONG LeapSecondFlags; // 0x7c0
		struct
		{
			ULONG SixtySecondEnabled : 1; // 0x7c0
			ULONG Reserved : 31;		  // 0x7c0
		};
	};
	ULONG NtGlobalFlag2; // 0x7c4
} PEB, *PPEB;

typedef struct _PEB_LDR_DATA
{
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY
{
	PVOID Reserved1[2];
	LIST_ENTRY InMemoryOrderLinks;
	PVOID Reserved2[2];
	PVOID DllBase;
	PVOID Reserved3[2];
	UNICODE_STRING FullDllName;
	BYTE Reserved4[8];
	PVOID Reserved5[3];
	__C89_NAMELESS union
	{
		ULONG CheckSum;
		PVOID Reserved6;
	};
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) \
	{                                             \
		(p)->Length = sizeof(OBJECT_ATTRIBUTES);  \
		(p)->RootDirectory = r;                   \
		(p)->Attributes = a;                      \
		(p)->ObjectName = n;                      \
		(p)->SecurityDescriptor = s;              \
		(p)->SecurityQualityOfService = NULL;     \
	}


#define OBJ_INHERIT 0x00000002L
#define OBJ_PERMANENT 0x00000010L
#define OBJ_EXCLUSIVE 0x00000020L
#define OBJ_CASE_INSENSITIVE 0x00000040L
#define OBJ_OPENIF 0x00000080L
#define OBJ_OPENLINK 0x00000100L
#define OBJ_KERNEL_HANDLE 0x00000200L
#define OBJ_FORCE_ACCESS_CHECK 0x00000400L
#define OBJ_IGNORE_IMPERSONATED_DEVICEMAP 0x00000800L
#define OBJ_DONT_REPARSE 0x00001000L
#define OBJ_VALID_ATTRIBUTES 0x00001FF2L

#define FILE_SUPERSEDE 0x00000000
#define FILE_OPEN 0x00000001
#define FILE_CREATE 0x00000002
#define FILE_OPEN_IF 0x00000003
#define FILE_OVERWRITE 0x00000004
#define FILE_OVERWRITE_IF 0x00000005
#define FILE_MAXIMUM_DISPOSITION 0x00000005

#define STATUS_SUCCESS (0L)

typedef struct _LDR_RESOURCE_INFO
{
	ULONG_PTR Type;
	ULONG_PTR Name;
	ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

PVOID RtlAllocateHeap(
	PVOID HeapHandle,
	ULONG Flags,
	SIZE_T Size);

BOOLEAN RtlFreeHeap(
	PVOID HeapHandle,
	ULONG Flags,
	PVOID BaseAddress);


NTSTATUS RtlAdjustPrivilege(
    _In_ ULONG Privilege,
    _In_ BOOLEAN Enable,
    _In_ BOOLEAN Client,
    _Out_ PBOOLEAN WasEnabled);

NTSTATUS NtDeviceIoControlFile(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength);

NTSTATUS NtCreateFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength);

NTSTATUS NtWriteFile(
	HANDLE FileHandle,
	HANDLE Event,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset,
	PULONG Key);

NTSTATUS NtCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    );

NTSTATUS NtSetValueKey(
  IN HANDLE               KeyHandle,
  IN PUNICODE_STRING      ValueName,
  IN ULONG                TitleIndex OPTIONAL,
  IN ULONG                Type,
  IN PVOID                Data,
  IN ULONG                DataSize );

NTSTATUS NtDeleteKey(HANDLE KeyHandle);

NTSTATUS NtDeleteFile(_In_ POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS RtlUnicodeStringPrintf(
   PUNICODE_STRING  DestinationString,
    PCWSTR pszFormat,
        ...              
);

NTSTATUS NtLoadDriver(IN PUNICODE_STRING DriverServiceName);

NTSTATUS NtUnloadDriver(IN PUNICODE_STRING DriverServiceName);

NTSTATUS NtClose(HANDLE Handle);

NTSTATUS LdrFindResource_U(
	HMODULE hmod,
	const LDR_RESOURCE_INFO *info,
	ULONG level,
	const PIMAGE_RESOURCE_DATA_ENTRY *entry);

NTSTATUS NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);

int printf(const char *format, ...);

#define PATTERN_SEARCH_RANGE 0xBFFFFF

typedef struct _BIN_RESOURCE
{
	PVOID DataBuffer;
	DWORD BufferSizeBytes;

} BIN_RESOURCE, *PBIN_RESOURCE;

typedef BOOL (*ReadMemoryDriverRoutine)(HANDLE driverHandle, PVOID startAddress, _Out_ PVOID buffer, size_t sizeInBytes);
typedef BOOL (*WriteMemoryDriverRoutine)(HANDLE driverHandle, PVOID startAddress, _In_ PVOID buffer, size_t sizeInBytes);

typedef struct _VULNERABLE_DRIVER_ENTRY
{
	/* We use hardcoded strings three times because like this we dont need to
	do any annoying string concats with native or selfmade libc apis*/
	LPWSTR DriverDeviceSymbolicLinkPath;
	LPWSTR DriverName;
	LPWSTR FullDriverFilePath; 
	LPWSTR DriverRegistryKey; 
	
	/*The r/w routines for each driver, only the ones non-NULL are valid for the driver*/

	ReadMemoryDriverRoutine VirtualReadRoutine;
	WriteMemoryDriverRoutine VirtualWriteRoutine;
	ReadMemoryDriverRoutine PhysicalReadRoutine;
	WriteMemoryDriverRoutine PhysicalWriteRoutine;
} VULNERABLE_DRIVER_ENTRY, *PVULNERABLE_DRIVER_ENTRY;


// we do assume name to be a direct string literal, beware of the triple eval...
#define INITIALIZE_VULNERABLE_DRIVER_ENTRY(driverDeviceLinkPath, name, virtualReadRoutine, virtualWriteRoutine, physicalReadRoutine, physicalWriteRoutine) \
{																		\
	driverDeviceLinkPath,												\
	name,																\
	WIDE_STRING(DRIVER_INSTALLATION_PATH) name L".sys",					\
	L"\\registry\\machine\\SYSTEM\\CurrentControlSet\\Services\\" name,	\
	virtualReadRoutine,													\
	virtualWriteRoutine,												\
	physicalReadRoutine,												\
	physicalWriteRoutine												\
}

#include "DriverLoaderHelpers.h"
#include "Drivers/WinIo.h"