#pragma once


#define REQUEST_RAW_DATA_BUFFER_SIZE 1024

BOOL HlParseRequestMsg(char *request, size_t requestBufferSize, _Out_ BYTE *message, size_t dstMaxSize, size_t requestContentLength);

BOOL HlInitListenSocket(SOCKET *pServerSocket, PSOCKADDR_IN pServerAddress, PSOCKADDR_IN pClientAddress);

BOOL HlRecieveRequest(SOCKET *pServerSocket, SOCKET *pClientSocket, PSOCKADDR_IN pClientAddress, int ClientAddressLength, _Out_ PBYTE pBuffer, int bufferSize, _Out_ PINT pLength);
