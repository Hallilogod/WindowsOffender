#pragma once


#pragma warning (disable : 4100)

#include "../Shared.h"

#define DRIVER_POOL_ALLOCATION_TAG 'YYKK'

// #define HIDEMYASS

#include <fltKernel.h>
#include <wdmsec.h>
#include <ntifs.h>
#include <ntstrsafe.h>
#include <dontuse.h>

/*
Function prefixes:

Ih - IOCTLHandling
Ff - File
ing
Rf - RegistryFiltering
U - Utils

*/
#define STRINGIFY_INTERNL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNL(x)

#ifdef DEBUG

#define Debug_Log_Internal DbgPrint

#else 

#define Debug_Log_Internal __noop

#endif

// We dont define anything that prints regardless of the DBG macro because the driver should not make any noise in release builds

#define DBGLOG(symbol, ...) 	do { Debug_Log_Internal(symbol " <" __FILE__ ":" STRINGIFY(__LINE__) "> " __VA_ARGS__); Debug_Log_Internal("\n"); } while(FALSE)

#define DBGRAW(...)		Debug_Log_Internal(__VA_ARGS__)

#define DBGWARN(...)	DBGLOG("[!]", __VA_ARGS__)

#define DBGERROR(...)	DBGLOG("[X]", __VA_ARGS__)

#define DBGOK(...)		DBGLOG("[+]", __VA_ARGS__)

#define DBGINFO(...)	DBGLOG("[i]", __VA_ARGS__)

#define DBGERRORNTSTATUS(Function, ntStatus)	DBGERROR("%s failed with NTSTATUS %08X", Function, ntStatus)

// Undocumented stuff


NTSTATUS DriverObjectCreateClose(PDEVICE_OBJECT pDeviceObject, PIRP irp);

NTSTATUS InitializeFiltering(PDRIVER_OBJECT pDriverObject, PFLT_FILTER* pFltFilter);

NTSTATUS UninitializeFiltering(PFLT_FILTER pFltFilter);

void WinlogonWaitThreadRoutine(PVOID StartContext);

void DriverUnload(PDRIVER_OBJECT pDriverObj);

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pUnicodeString);

#include "Data.h"
#include "Undocumented.h"
#include "Utils.h"
#include "FileFiltering.h"
#include "RegistryFiltering.h"
#include "IOCTLHandling.h"