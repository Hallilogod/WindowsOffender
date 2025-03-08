#pragma once


/*
    HGLOBAL unlockMe: resource data.
    PVOID binaryData: a pointer to the data buffer.
    DWORD size: the size of the buffer in bytes.
*/
typedef struct _BIN_RESOURCE
{
    HGLOBAL unlockMe;
    PVOID binaryData;
    DWORD size;
    
} BIN_RESOURCE, *PBIN_RESOURCE;


/*
CreateServiceA Wrapper to make creating services quicker.

@param scmHandle A handle to the service control manager.
@param imagePath The fully qualified path to the service binary file.
@param serviceName The name of the service to install with a maximum length of 256 characters.
@param startType The service start options.
@param isKernelDriver A boolean indicating whether the service should be created as a kernel driver (TRUE), or a win32 own process (FALSE).

@return CreateServiceA's return value.
*/
SC_HANDLE InsUtCreateServiceEasy(SC_HANDLE scmHandle, char *imagePath, char *serviceName, DWORD startType, BOOL isKernelDriver);

/*
Returns a PBIN_RESOURCE for the requested resource.

@param resourceName: Name of the resource.
@param type: Type of the resource.

@return a pointer to the BIN_RESOURCE on success, NULL otherwise.
*/
PBIN_RESOURCE InsUtGetBinResource(LPCSTR resourceName, PSTR type);

/*
writes the data of a BIN_RESOURCE to a file.

@param pBinResource: BIN_RESOURCE pointer to the BIN_RESOURCE to write.
@param fullFilePath: Full path of the file to write the data to.

@return TRUE on success, FALSE otherwise.
*/
BOOL InsUtWriteBinResourceToFile(
    PBIN_RESOURCE pBinResource,
    PSTR fullFilePath);

/* 
Frees a BIN_RESOURCE.
@param pBinResource: PBIN_RESOURCE to be freed that was previously aquired trough GetBinResource
@return nothing.
*/
#define InsUtFreeBinResource(pBinResource) \
    UnlockResource(pBinResource->unlockMe); \
    free(pBinResource)

/* Opens or creates the key and sets the value
   ther too many arguments i dont wanna explain, just read the code
*/
BOOL InsUtEasyRegSetKeyValueExA64(
    HKEY rootKey,
    PSTR subKey,
    PCSTR valueName,
    DWORD datatype,
    const PBYTE data,
    DWORD dataSizeInBytes,
    DWORD keyCreateOptions,
    BOOL deleteOriginal);



/*
Enables a Privilege in the current process token.
@param PrivilegeName: name of the Privilege to enable for the own token.
@return TRUE on success, FALSE otherwise.
*/
BOOL InsUtEnablePrivilegeForCurrentProcessToken(LPSTR privilegeName);