#include "Installer.h"


SC_HANDLE InsUtCreateServiceEasy(SC_HANDLE scmHandle, char *imagePath, char *serviceName, DWORD startType, BOOL isKernelDriver)
{

    return CreateServiceA(scmHandle,
                          serviceName,
                          NULL,
                          SERVICE_QUERY_STATUS | DELETE | SERVICE_START,
                          isKernelDriver ? SERVICE_KERNEL_DRIVER : SERVICE_WIN32_OWN_PROCESS,
                          startType,
                          SERVICE_ERROR_IGNORE,
                          imagePath,
                          NULL, NULL, NULL, NULL, NULL);
}


PBIN_RESOURCE InsUtGetBinResource(LPCSTR resourceName, PSTR type)
    
{
    HRSRC hResource = FindResourceA(NULL, resourceName, type);
    if (hResource == NULL)
    {
        return NULL;
    }

    HGLOBAL hResourceData = LoadResource(NULL, hResource);
    if (hResourceData == NULL)
    {
        return NULL;
    }

    LPVOID binData = LockResource(hResourceData);
    if (binData == NULL)
    {
        return NULL;
    }

    DWORD dataSize = SizeofResource(NULL, hResource);

    PBIN_RESOURCE resource = (PBIN_RESOURCE)malloc(sizeof(BIN_RESOURCE));
    resource->unlockMe = hResourceData;
    resource->binaryData = binData;
    resource->size = dataSize;

    return resource;
}


BOOL InsUtWriteBinResourceToFile(PBIN_RESOURCE pBinResource, PSTR fullFilePath)
{

    HANDLE hFile = CreateFileA(
        fullFilePath,
        FILE_GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    DWORD numBytesWritten;

    BOOL ret = WriteFile(
        hFile,
        pBinResource->binaryData,
        pBinResource->size,
        &numBytesWritten,
        NULL);

    CloseHandle(hFile);

    if(!ret)
    {
        DeleteFileA(fullFilePath);
    }

    return ret;
}


BOOL InsUtEasyRegSetKeyValueExA64(
    HKEY rootKey,
    PSTR subKey,
    PCSTR valueName,
    DWORD datatype,
    const PBYTE data,
    DWORD dataSizeInBytes,
    DWORD keyCreateOptions,
    BOOL deleteOriginal)
{
    HKEY hKey;

    if (deleteOriginal)
    {
        RegDeleteTreeA(rootKey,
                        subKey);
    }

    if (RegCreateKeyExA(
            rootKey,
            subKey,
            0,
            NULL,
            keyCreateOptions,
            KEY_WRITE | KEY_WOW64_64KEY,
            NULL,
            &hKey,
            NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (RegSetValueExA(
            hKey,
            valueName,
            0,
            datatype,
            data,
            dataSizeInBytes) != ERROR_SUCCESS)

    {
        RegCloseKey(hKey);

        RegDeleteKeyExA(rootKey, subKey, KEY_WOW64_64KEY, 0);
        
        return FALSE;
    }

    RegCloseKey(hKey);
    
    return TRUE;
}



BOOL InsUtEnablePrivilegeForCurrentProcessToken(LPSTR privilegeName)
{
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueA(NULL, privilegeName, &tp.Privileges[0].Luid))
    {

        return FALSE;
    }

    HANDLE ownProcess;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &ownProcess))
    {
        return FALSE;
    }
    if (!AdjustTokenPrivileges(ownProcess, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
    {
        CloseHandle(ownProcess);
        return FALSE;
    }
    if (GetLastError() != ERROR_SUCCESS)
    {
        CloseHandle(ownProcess);
        return FALSE;
    }
    CloseHandle(ownProcess);
    return TRUE;
}