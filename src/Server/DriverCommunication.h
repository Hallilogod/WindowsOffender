#pragma once

SERVER_STATUS_CODE SendIOCTL(
    DWORD ioctlCode,
    PVOID ioctlInputBuffer,
    DWORD ioctlInputBufferSize,
    _Out_ PVOID outputBuffer,
    DWORD outputBufferSize,
    _Out_ LPDWORD bytesReturned);
