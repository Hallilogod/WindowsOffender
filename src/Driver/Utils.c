#include "Driver.h"

NTSTATUS UOpenProcessbyPid(ULONG pid, PHANDLE pHandle)
{
	NTSTATUS status;
	PEPROCESS pEprocess;

	status = PsLookupProcessByProcessId(UlongToHandle(pid), &pEprocess);

	if (!NT_SUCCESS(status))
		return status;


	status = ObOpenObjectByPointer(pEprocess, OBJ_KERNEL_HANDLE, NULL,
		SYNCHRONIZE, NULL, KernelMode, pHandle);

	ObDereferenceObject(pEprocess);

	return status;
}


NTSTATUS UTerminateProcessByPid(ULONG pid)
{
	NTSTATUS status;
	HANDLE processHandle;


	status = UOpenProcessbyPid(pid, &processHandle);

	if (NT_SUCCESS(status))
	{
		status = ZwTerminateProcess(processHandle, STATUS_SUCCESS);

		ZwClose(processHandle);
		
	}

	return status;
}

BOOLEAN UEndsWith(PUNICODE_STRING pUnicodeString, PWSTR checkString)
{
	SIZE_T checkStringLengthBytes = wcslen(checkString) * sizeof(WCHAR);
	// If checkString is longer than the unicode string it cant end with it
	if (checkStringLengthBytes > pUnicodeString->Length)
	{
		return FALSE;
	}

	// Check if pUnicodeString's Buffer ends with the same data as checkString
	if (RtlCompareMemory(
		(PUINT8)pUnicodeString->Buffer + (pUnicodeString->Length - checkStringLengthBytes),
		checkString,
		checkStringLengthBytes) == checkStringLengthBytes)
	{
		return TRUE;
	}

	return FALSE;

}

BOOLEAN UStartsWith(PUNICODE_STRING pUnicodeString, PWSTR checkString)
{
	SIZE_T checkStringLengthBytes = wcslen(checkString) * sizeof(WCHAR);

	// If checkString is longer than the unicode string it cant start with it
	if (checkStringLengthBytes > pUnicodeString->Length)
	{
		return FALSE;
	}

	// Check if pUnicodeString's Buffer starts with the same data as checkString
	if (RtlCompareMemory(
		pUnicodeString->Buffer,
		checkString,
		checkStringLengthBytes) == checkStringLengthBytes)
	{
		return TRUE;
	}

	return FALSE;

}

BOOLEAN UEqualMemoryCaseInsensitive(const VOID* source1, const VOID* source2, SIZE_T lengthInBytes)
{
	PUINT8 firstSource = (PUINT8)source1;
	PUINT8 secondSource = (PUINT8)source2;


	for (SIZE_T i = 0; i < lengthInBytes; i++)
	{
		if (tolower(firstSource[i]) != tolower(secondSource[i]))
		{
			return FALSE;
		}
	}

	return TRUE;

}

BOOLEAN UCompareCombinedUnicodeStringAndUnicodeStringToString(PUNICODE_STRING string1, PUNICODE_STRING string2, PCWSTR compareString)
{

	SIZE_T compareStringSize = wcslen(compareString) * sizeof(WCHAR);
	SIZE_T string1Size = string1->Length;
	SIZE_T string2Size = string2->Length;


	if ((string1Size + string2Size) != compareStringSize)
	{
		return FALSE;
	}
	

	if (UEqualMemoryCaseInsensitive(string1->Buffer, compareString, string1Size))
	{
		if (UEqualMemoryCaseInsensitive(string2->Buffer, (PWSTR)((PUINT8)compareString + string1Size), string2Size))
		{
			return TRUE;
		}
	}

	return FALSE;

}

BOOLEAN UCompareCombinedUnicodeStringAndStringToString(PUNICODE_STRING string1, PWSTR string2, PWSTR compareString)
{

	SIZE_T compareStringSize = wcslen(compareString) * sizeof(WCHAR);
	SIZE_T string1Size = string1->Length;
	SIZE_T string2Size = wcslen(string2) * sizeof(WCHAR);

	if ((string1Size + string2Size) != compareStringSize)
	{
		return FALSE;
	}


	if (UEqualMemoryCaseInsensitive(string1->Buffer, compareString, string1Size))
	{
		if (UEqualMemoryCaseInsensitive(string2, (PWSTR)((PUINT8)compareString + string1Size), string2Size))
		{
			return TRUE;
		}
	}

	return FALSE;

}

BOOLEAN UCompareCombinedUnicodeStringWithMiddleCharAndStringToUnicodeString(PUNICODE_STRING string1, WCHAR middleChar, PWSTR string2, PUNICODE_STRING compareString)
{
	SIZE_T compareStringSize = compareString->Length;
	SIZE_T string1Size = string1->Length;
	SIZE_T string2Size = wcslen(string2) * sizeof(WCHAR);

	if ((string1Size + string2Size + sizeof(WCHAR) /*middle char*/) != compareStringSize)
	{
		return FALSE;
	}

	// Check if middleChar is correct
	if (*(PWSTR)((PUINT8)compareString->Buffer + string1Size) != middleChar)
	{
		return FALSE;
	}

	if (UEqualMemoryCaseInsensitive(string1->Buffer, compareString->Buffer, string1Size))
	{

		if (UEqualMemoryCaseInsensitive(string2, (PWSTR)((PUINT8)compareString->Buffer + (string1Size + sizeof(WCHAR) /*skip middle char*/)), string2Size))
		{
			return TRUE;
		}
	}


	return FALSE;
}

NTSTATUS UForceDeleteFile_IoComplete(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID ctx) {

	UNREFERENCED_PARAMETER(pDeviceObject);
	UNREFERENCED_PARAMETER(ctx);

	pIrp->UserIosb->Status = pIrp->IoStatus.Status;
	pIrp->UserIosb->Information = pIrp->IoStatus.Information;

	KeSetEvent(pIrp->UserEvent, IO_NO_INCREMENT, FALSE);
	IoFreeIrp(pIrp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS UForceDeleteFile_SendIrp(PFILE_OBJECT file_object) {

	KEVENT event;
	PDEVICE_OBJECT device_object = IoGetBaseFileSystemDeviceObject(file_object);

	PIRP pIrp = IoAllocateIrp(device_object->StackSize, FALSE);

	// Set the complete routine that will free the IRP and signal the event
	KeInitializeEvent(&event, SynchronizationEvent, FALSE);

	IoSetCompletionRoutine(
		pIrp,
		UForceDeleteFile_IoComplete,
		&event,
		TRUE, TRUE, TRUE);



	FILE_DISPOSITION_INFORMATION_EX dispositionInformation;
	dispositionInformation.Flags =
		FILE_DISPOSITION_DELETE |
		FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE |
		FILE_DISPOSITION_POSIX_SEMANTICS;


	IO_STATUS_BLOCK io_status_block;
	pIrp->AssociatedIrp.SystemBuffer = &dispositionInformation;
	pIrp->UserEvent = &event;

	pIrp->UserIosb = &io_status_block;
	pIrp->Tail.Overlay.OriginalFileObject = file_object;
	pIrp->Tail.Overlay.Thread = KeGetCurrentThread();
	pIrp->Flags = IRP_WRITE_OPERATION;
	pIrp->RequestorMode = KernelMode;

	PIO_STACK_LOCATION stack_location = IoGetNextIrpStackLocation(pIrp);
	stack_location->MajorFunction = IRP_MJ_SET_INFORMATION;
	stack_location->DeviceObject = device_object;
	stack_location->FileObject = file_object;
	stack_location->Flags |= SL_FORCE_DIRECT_WRITE;

	stack_location->Parameters.SetFile.Length = sizeof(FILE_DISPOSITION_INFORMATION_EX);
	stack_location->Parameters.SetFile.FileInformationClass = FileDispositionInformationEx;
	stack_location->Parameters.SetFile.FileObject = file_object;

	IoCallDriver(device_object, pIrp);

	KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, NULL);

	return pIrp->IoStatus.Status;
}

NTSTATUS UForceDeleteFile(PCWSTR fullFilePath)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES fileObjAttribs;
	HANDLE fileHandle;
	IO_STATUS_BLOCK ioBlock; 
	UNICODE_STRING filePath;


	RtlInitUnicodeString(&filePath, fullFilePath);

	InitializeObjectAttributes(&fileObjAttribs, &filePath,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);

	status = IoCreateFileEx(&fileHandle,
		SYNCHRONIZE,
		&fileObjAttribs, &ioBlock,
		NULL, FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0,
		CreateFileTypeNone, NULL,
		IO_IGNORE_SHARE_ACCESS_CHECK,
		NULL);



	if (!NT_SUCCESS(status))
	{
		return status;
	}

	PFILE_OBJECT fileObject;
	status = ObReferenceObjectByHandleWithTag(fileHandle,
		SYNCHRONIZE, *IoFileObjectType,
		KernelMode, 'eliF',
		&fileObject, NULL);

	if (!NT_SUCCESS(status))
	{

		ObCloseHandle(fileHandle, KernelMode);
		return status;
	}

	fileObject->SectionObjectPointer->ImageSectionObject = 0;
	fileObject->SharedDelete = TRUE;
	fileObject->DeleteAccess = TRUE;
	fileObject->WriteAccess = TRUE;
	fileObject->ReadAccess = TRUE;
	fileObject->DeletePending = FALSE;
	fileObject->Busy = FALSE;


	if (!MmFlushImageSection(fileObject->SectionObjectPointer, MmFlushForDelete))
	{
		DBGWARN("Failed to flush image section");
	}

	status = UForceDeleteFile_SendIrp(fileObject);

	ObfDereferenceObject(fileObject);
	ObCloseHandle(fileHandle, KernelMode);

	return status;
}

NTSTATUS UDeleteKeyRecursively(HANDLE keyRoot)
{
	NTSTATUS status;
	PKEY_FULL_INFORMATION keyFullInfo = NULL;
	ULONG length = sizeof(KEY_FULL_INFORMATION);
	ULONG numSubKeys;
	ULONG lengthOfKeyName;
	LPWSTR subKey = NULL;
	PKEY_BASIC_INFORMATION keyBasicInfo = NULL;
	ULONG index = 0;
	HANDLE srcSubKey = NULL;
	OBJECT_ATTRIBUTES objectAttributes;



	if (!keyRoot) {

		status = STATUS_INVALID_PARAMETER;
		goto _Exit_UDeleteKeyRecursive;
	}

	//
	// Query the source key for information about number of subkeys and max
	// length needed for subkey name.
	//
	do {
		if (keyFullInfo) {

			ExFreePool(keyFullInfo);
		}

		keyFullInfo = ExAllocatePool2(POOL_FLAG_NON_PAGED, length, DRIVER_POOL_ALLOCATION_TAG);

		if (!keyFullInfo) {


			status = STATUS_INSUFFICIENT_RESOURCES;
			goto _Exit_UDeleteKeyRecursive;
		}

		status = ZwQueryKey(keyRoot,
			KeyFullInformation,
			keyFullInfo,
			length,
			&length);

	} while (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW);

	if (!NT_SUCCESS(status)) {


		goto _Exit_UDeleteKeyRecursive;
	}

	numSubKeys = keyFullInfo->SubKeys;
	lengthOfKeyName = keyFullInfo->MaxNameLen + sizeof(WCHAR);

	if (numSubKeys) {

		//
		// Allocate buffer for subkey name
		//
		subKey = ExAllocatePool2(POOL_FLAG_NON_PAGED,
			lengthOfKeyName, DRIVER_POOL_ALLOCATION_TAG);

		if (!subKey) {



			status = STATUS_INSUFFICIENT_RESOURCES;
			goto _Exit_UDeleteKeyRecursive;
		}

		//
		// Now Enumerate all of the subkeys
		//
		index = numSubKeys - 1;
		length = sizeof(KEY_BASIC_INFORMATION);
		do {

			UNICODE_STRING subKeyName;

			do {
				if (keyBasicInfo) {

					ExFreePool(keyBasicInfo);
				}

				keyBasicInfo = ExAllocatePool2(POOL_FLAG_NON_PAGED,
					length, DRIVER_POOL_ALLOCATION_TAG);

				if (!keyBasicInfo) {

					status = STATUS_INSUFFICIENT_RESOURCES;
					goto _Exit_UDeleteKeyRecursive;
				}

				//
				// Enumerate the index'th subkey
				//
				status = ZwEnumerateKey(keyRoot,
					index,
					KeyBasicInformation,
					keyBasicInfo,
					length,
					&length);

			} while (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW);

			if (NT_SUCCESS(status)) {

				RtlZeroMemory(subKey, lengthOfKeyName);
				RtlStringCbCopyNW(subKey, lengthOfKeyName, keyBasicInfo->Name, keyBasicInfo->NameLength);
				RtlInitUnicodeString(&subKeyName, subKey);

				//
				// Open a handle to the the current root's subkey.
				//
				InitializeObjectAttributes(&objectAttributes,
					&subKeyName,
					(OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
					keyRoot,
					(PSECURITY_DESCRIPTOR)NULL);

				status = ZwOpenKey(&srcSubKey,
					KEY_ALL_ACCESS,
					&objectAttributes);

				if (!NT_SUCCESS(status)) {

					goto _Exit_UDeleteKeyRecursive;
				}

				//
				// Delete this key's subtree (recursively).
				//
				status = UDeleteKeyRecursively(srcSubKey);

				ZwClose(srcSubKey);
				srcSubKey = NULL;
			}

			index--;

		} while (status != STATUS_NO_MORE_ENTRIES && (LONG)index >= 0);

		if (status == STATUS_NO_MORE_ENTRIES) {

			status = STATUS_SUCCESS;
		}
	}

	ZwDeleteKey(keyRoot);

_Exit_UDeleteKeyRecursive:

	if (srcSubKey)
	{
		ZwClose(srcSubKey);
	}

	if (keyFullInfo)
	{
		ExFreePool(keyFullInfo);
	}

	if (subKey)
	{
		ExFreePool(subKey);
	}

	if (keyBasicInfo)
	{
		ExFreePool(keyBasicInfo);
	}

	return status;
}

NTSTATUS UGetFirstWinlogonInstancePid(_Out_ PULONG pWinlogonPid)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG systemInformationBufferSizeBytes = 0;
	PVOID systemInformationBuffer = NULL;

	*pWinlogonPid = 0;

	// Get the required buffer size
	status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &systemInformationBufferSizeBytes);

	if (status != STATUS_INFO_LENGTH_MISMATCH || systemInformationBufferSizeBytes == 0)
	{
		return status;
	}

	// Allocate memory for process list
	systemInformationBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, systemInformationBufferSizeBytes, DRIVER_POOL_ALLOCATION_TAG);

	if (systemInformationBuffer == NULL)
	{
		DBGERRORNTSTATUS("ExAllocatePoolWithTag", status);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = ZwQuerySystemInformation(SystemProcessInformation, systemInformationBuffer, systemInformationBufferSizeBytes, &systemInformationBufferSizeBytes);

	if (!NT_SUCCESS(status))
	{
		ExFreePoolWithTag(systemInformationBuffer, DRIVER_POOL_ALLOCATION_TAG);

		DBGERRORNTSTATUS("ZwQuerySystemInformation", status);
		return status;
	}


	ULONG firstWinlogonInstancePid = 0;

	PSYSTEM_PROCESSES processEntry = (PSYSTEM_PROCESSES)systemInformationBuffer;

	// Iterate through all processes
	do {
		if (processEntry->ProcessName.Length) 
		{
			// Check if process name matches "winlogon.exe"
			if (_wcsnicmp(processEntry->ProcessName.Buffer, L"winlogon.exe", processEntry->ProcessName.Length / sizeof(WCHAR)) == 0)
			{
				PEPROCESS targetPeProcess = NULL;

				status = PsLookupProcessByProcessId(ULongToHandle((ULONG)processEntry->ProcessId), &targetPeProcess);

				if (!NT_SUCCESS(status))
				{
					DBGERRORNTSTATUS("PsLookupProcessByProcessId", status);

					ExFreePoolWithTag(systemInformationBuffer, DRIVER_POOL_ALLOCATION_TAG);

					return status;
				}

				if (PsGetProcessSessionId(targetPeProcess) == 1)
				{
					firstWinlogonInstancePid = (ULONG)processEntry->ProcessId;

					ObDereferenceObject(targetPeProcess);

					break; // Found first Winlogon instance, exit loop
				}

				ObDereferenceObject(targetPeProcess);
			}
		}
		processEntry = (PSYSTEM_PROCESSES)((PUINT8)processEntry + processEntry->NextEntryDelta);
	} while (processEntry->NextEntryDelta);

	ExFreePoolWithTag(systemInformationBuffer, DRIVER_POOL_ALLOCATION_TAG);

	if (firstWinlogonInstancePid)
	{
		*pWinlogonPid = firstWinlogonInstancePid;
		return STATUS_SUCCESS;
	}

	return STATUS_NOT_FOUND;
}

NTSTATUS UInjectShellcode(
	ULONG targetProcessPid,
	PVOID shellcodeBuffer,
	SIZE_T shellcodeBufferSizeBytes,
	PVOID shellcodeParameterBuffer,
	SIZE_T shellcodeParameterBufferSizeBytes)
{
	HANDLE targetProcessHandle = NULL;
	PVOID remoteShellcodeBufferBase = NULL;
	PVOID remoteShellcodeParameterBase = NULL;
	SIZE_T remoteShellcodeBufferRegionSize = shellcodeBufferSizeBytes;
	SIZE_T remoteShellcodeParameterBufferRegionSize = shellcodeParameterBufferSizeBytes;
	SIZE_T shellcodeBytesWritten = 0;
	SIZE_T shellcodeParameterBytesWritten = 0;
	ULONG shellcodeProtection = PAGE_READWRITE;
	HANDLE remoteThreadHandle = NULL;
	CLIENT_ID remoteThreadClientId = { 0 };
	PEPROCESS targetPeProcess = NULL;
	NTSTATUS status = STATUS_SUCCESS;

	
	NT_ASSERT(targetProcessPid != 0 && shellcodeBufferSizeBytes > 0 && shellcodeBuffer != NULL);

	status = UOpenProcessbyPid(targetProcessPid, &targetProcessHandle);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("UOpenProcessbyPid", status);

		targetProcessHandle = NULL;

		goto _UInjectShellcode_Cleanup;
	}

	status = ZwAllocateVirtualMemory(
		targetProcessHandle,
		&remoteShellcodeBufferBase,
		0,
		&remoteShellcodeBufferRegionSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ZwAllocateVirtualMemory", status);

		remoteShellcodeBufferBase = NULL;

		goto _UInjectShellcode_Cleanup;
	}

	status = ZwAllocateVirtualMemory(
		targetProcessHandle,
		&remoteShellcodeParameterBase,
		0,
		&remoteShellcodeParameterBufferRegionSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ZwAllocateVirtualMemory", status);

		remoteShellcodeParameterBase = NULL;

		goto _UInjectShellcode_Cleanup;
	}

	status = PsLookupProcessByProcessId(ULongToHandle(targetProcessPid), &targetPeProcess);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("PsLookupProcessByProcessId", status);

		targetPeProcess = NULL;

		goto _UInjectShellcode_Cleanup;
	}

	status = MmCopyVirtualMemory(
		PsGetCurrentProcess(),
		shellcodeBuffer,
		targetPeProcess,
		remoteShellcodeBufferBase,
		shellcodeBufferSizeBytes,
		KernelMode,
		&shellcodeBytesWritten);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("MmCopyVirtualMemory", status);

		goto _UInjectShellcode_Cleanup;
	}


	status = MmCopyVirtualMemory(
		PsGetCurrentProcess(),
		shellcodeParameterBuffer,
		targetPeProcess,
		remoteShellcodeParameterBase,
		shellcodeParameterBufferSizeBytes,
		KernelMode,
		&shellcodeParameterBytesWritten);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("MmCopyVirtualMemory", status);

		goto _UInjectShellcode_Cleanup;
	}

	status = ZwProtectVirtualMemory(
		targetProcessHandle,
		&remoteShellcodeBufferBase,
		(PULONG)(&shellcodeBufferSizeBytes),
		PAGE_EXECUTE_READ,
		&shellcodeProtection);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ZwProtectVirtualMemory", status);

		goto _UInjectShellcode_Cleanup;
	}

	ObDereferenceObject(targetPeProcess);
	targetPeProcess = NULL;

	status = RtlCreateUserThread(
		targetProcessHandle,
		NULL,
		FALSE,
		0,
		0,
		0,
		remoteShellcodeBufferBase,
		remoteShellcodeParameterBase, // Thread Parameter
		&remoteThreadHandle,
		&remoteThreadClientId);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("RtlCreateUserThread", status);

		remoteThreadHandle = NULL;

		goto _UInjectShellcode_Cleanup;
	}

	status = ZwWaitForSingleObject(remoteThreadHandle, FALSE, NULL);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ZwWaitForSingleObject", status);

		goto _UInjectShellcode_Cleanup;
	}

_UInjectShellcode_Cleanup:

	if (remoteShellcodeParameterBase != NULL)
	{
		ZwFreeVirtualMemory(targetProcessHandle, &remoteShellcodeParameterBase, &shellcodeParameterBufferSizeBytes, MEM_DECOMMIT | MEM_RELEASE);
	}

	if(remoteShellcodeBufferBase != NULL)
	{
		ZwFreeVirtualMemory(targetProcessHandle, &remoteShellcodeBufferBase, &shellcodeBufferSizeBytes, MEM_DECOMMIT | MEM_RELEASE);
	}

	if (targetPeProcess != NULL)
	{
		ObDereferenceObject(targetPeProcess);
	}
	
	if (remoteThreadHandle != NULL)
	{
		ZwClose(remoteThreadHandle);
	}

	if(targetProcessHandle != NULL)
	{
		ZwClose(targetProcessHandle);
	}


	return status;
}