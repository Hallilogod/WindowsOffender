#pragma once


BOOLEAN FfShouldHideFilePUS(PUNICODE_STRING filePath, PWSTR fileName);

BOOLEAN FfShouldHideFilePUSFullPath(PUNICODE_STRING fullFilePath);

BOOLEAN FfShouldHideFilePWC(PWCH fileNameToCheck);


/*
@brief inserts a Harddisk volume number into the string filePathToInsertVolumeNumber from filePathWithVolumeNumber.

@param filePathToInsertVolumeNumber A pointer to the unicode string starting with \Device\HarddiskVolumeX where X should be replaced
@param filePathWithVolumeNumber A pointer to the unicode string starting with \Device\HarddiskVolume<Number> where the correct number is contained

@return The appropriate NTSTATUS
*/
NTSTATUS FfInsertHarddiskVolumeNumber(PUNICODE_STRING filePathToInsertVolumeNumber, PUNICODE_STRING filePathWithVolumeNumber);



NTSTATUS FfSkipFileDirectoryInformationEntries(PFILE_DIRECTORY_INFORMATION pCurrentFileDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength);

NTSTATUS FfSkipFileFullDirectoryInformationEntries(PFILE_FULL_DIR_INFORMATION pCurrentFileFullDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength);

NTSTATUS FfSkipFileBothDirectoryInformationEntries(PFILE_BOTH_DIR_INFORMATION pCurrentFileBothDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength);

NTSTATUS FfSkipFileNamesInformationEntries(PFILE_NAMES_INFORMATION pCurrentFileNamesInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength);

NTSTATUS FfSkipFileIdBothDirectoryInformationEntries(PFILE_ID_BOTH_DIR_INFORMATION pCurrentFileIdBothDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength);

NTSTATUS FfSkipFileIdFullDirectoryInformationEntries(PFILE_ID_FULL_DIR_INFORMATION pCurrentFileIdFullDirInfo, PFLT_FILE_NAME_INFORMATION pFileNameInfo, ULONG bufferLength);


/*
@brief The Pre-Operation callback routine for the minifilter.
*/
FLT_PREOP_CALLBACK_STATUS FfCreatePreOperationCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);



NTSTATUS PfltFilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS flags);


/*
@brief Unregisters the File Minifilter for hiding files.

@param pFltFilter: A pointer to the filter initialized previously by FfRegisterFileFilter

@return None
*/
VOID FfUnregisterFileFilter(PFLT_FILTER pFltFilter);

/*
@brief Registers the File Minifilter for hiding files.

@param pDriverObject: A pointer to the driver's DRIVER_OBJECT
@param pFltFilter: A pointer to a caller-allocated variable that receives the filter pointer which needs to be passed to the filter unregistering routine

@return The appropriate NTSTATUS
*/
NTSTATUS FfRegisterFileFilter(PDRIVER_OBJECT pDriverObject, PFLT_FILTER* pFltFilter);


