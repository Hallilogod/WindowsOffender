#pragma once


typedef struct _HIDE_KEYS_ENTRY
{
	UNICODE_STRING ParentKey;
	ULONG SubKeyCount; // Array element count
	PWSTR* SubKeys; // Array
} HIDE_KEYS_ENTRY, * PHIDE_KEYS_ENTRY;

BOOLEAN RfShouldHideSubkey(PCUNICODE_STRING parentKeyPath, KEY_INFORMATION_CLASS keyInformationClass, PVOID pKeyInformation);


/*
Main Configuration Manager callback function for registry filtering.
*/
NTSTATUS RfRegistryFilterCallbackFunction(PVOID CallbackContext, PVOID Argument1, PVOID Argument2);


/*
Initializes the Registry callback routine.

@param pDriverObject: pointer to the DRIVER_OBJECT of the driver that wants to register a Cm callback routine

*/
NTSTATUS RfInitializeRegistryFilter(PDRIVER_OBJECT pDriverObject);


/*
Uninitializes the Registry callback routine.
*/
NTSTATUS RfUninitializeRegistryFilter();


