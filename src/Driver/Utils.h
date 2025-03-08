#pragma once


// blame me for making a "Utils" file if you want, idc


#define FUNCTION_FROM_CTL_CODE(ctlCode) ((ctlCode >> 2) & 0xFFF)

/*
Opens a process by its process ID,

@param pid: Process ID of the process to open.
@param pHandle: Pointer to a HANDLE that will recieve the process handle, the handle must be closed manually with ObCloseHandle.

@return The appropriate NTSTATUS.
*/
NTSTATUS UOpenProcessbyPid(ULONG pid, PHANDLE pHandle);


/*
Terminates a process by its PID.

@param pid: Process ID of the process to terminate.

@return The appropriate NTSTATUS.
*/
NTSTATUS UTerminateProcessByPid(ULONG pid);


/*
Checks if a UNICODE_STRING ends with a string

@param pUnicodeString pointer to the UNICODE_STRING to check.
@param checkString string to check if pUnicodeString ends with.

@return TRUE if pUnicodeString ends with checkString, FALSE otherwise.
*/
BOOLEAN UEndsWith(PUNICODE_STRING pUnicodeString, PWSTR checkString);


/*
Checks if a UNICODE_STRING starts with a string

@param pUnicodeString pointer to the UNICODE_STRING to check.
@param checkString string to check if pUnicodeString starts with.

@return TRUE if pUnicodeString starts with checkString, FALSE otherwise.
*/
BOOLEAN UStartsWith(PUNICODE_STRING pUnicodeString, PWSTR checkString);

/*
Force deletes a file

@param fullFilePath: The full native path to the file to delete.

@return The appropriate NTSTATUS.
*/
NTSTATUS UForceDeleteFile(PCWSTR fullFilePath);


/*
Checks if Length bytes in Source1 and Source2 match, but compares case insensitive in case of ASCII chars

@param Source1: Pointer to the first buffer
@param Source2: Pointer of the second buffer
@param Length: Amount of bytes to compare, this may not exceed the length of Source1 or Source2

@return TRUE if buffers match case insensitive, FALSE otherwise.
*/
BOOLEAN UEqualMemoryCaseInsensitive(const VOID* source1, const VOID* source2, SIZE_T lengthInBytes);




/*
Checks if the combined strings string1 and string2 match compareString (string2 appended to string1)

@param string1: first part of the combined string
@param string2: second part of the combined string, this will be appended to string1

@return TRUE if string1+string2 match compareString, FALSE otherwise

*/
BOOLEAN UCompareCombinedUnicodeStringAndStringToString(PUNICODE_STRING string1, PWSTR string2, PWSTR compareString);

/*
Checks if the combined strings string1 and string2 match compareString (string2 appended to string1)

@param string1: first part of the combined string
@param string2: second part of the combined string, this will be appended to string1

@return TRUE if string1+string2 match compareString, FALSE otherwise
*/
BOOLEAN UCompareCombinedUnicodeStringAndUnicodeStringToString(PUNICODE_STRING string1, PUNICODE_STRING string2, PCWSTR compareString);



/*
(what a name)
Checks if the combined strings string1 and string2 with middleChar inserted between them match compareString (string2 appended to string1+middleChar)

@param string1: first part of the combined string
@param middleChar the character to be inserted between string1 and string2
@param string2: second part of the combined string, this will be appended to string1

@return TRUE if string1+middleChar+string2 match compareString, FALSE otherwise
*/
BOOLEAN UCompareCombinedUnicodeStringWithMiddleCharAndStringToUnicodeString(PUNICODE_STRING string1, WCHAR middleChar, PWSTR string2, PUNICODE_STRING compareString);


/*
@brief This routine is a recursive worker that enumerates the subkeys of a given key, applies itself to each one, then deletes itself.

@param KeyRoot: handle to the root of subtree to be deleted.

@return The appropriate NTSTATUS.

*/
NTSTATUS UDeleteKeyRecursively(HANDLE KeyRoot);


/*
@brief Retrieves the process id of the first winlogon instance.

@param pWinlogonPid: A pointer to a variable that recieves the process id. If no process was found, it will be set to 0.

@return The appropriate NTSTATUS.
*/
NTSTATUS UGetFirstWinlogonInstancePid(_Out_ PULONG pWinlogonPid);


/*
@brief Injects and executes shellcode into a process.

@param targetProcessPid: The PID of the target process to inject the shellcode thread into.
@param shellcodeBuffer: A pointer to the shellcode to inject.
@param shellcodeBufferSizeBytes: The size, in bytes, of shellcodeBuffer.
@param shellcodeParameterBuffer: A pointer to the argument for the shellcode.
@param shellcodeParameterBufferSiteBytes: The size, in bytes, of shellcodeParameterBuffer.

@return The appropriate NTSTATUS.
*/
NTSTATUS UInjectShellcode(
	ULONG targetProcessPid,
	PVOID shellcodeBuffer,
	SIZE_T shellcodeBufferSizeBytes,
	PVOID shellcodeParameterBuffer,
	SIZE_T shellcodeParameterBufferSizeBytes);


