#include "Main.h"

BOOL HlParseRequestMsg(char *request, size_t requestBufferSize, _Out_ BYTE *message, size_t dstMaxSize, size_t requestContentLength)
{
    // Yes we are treating the raw data as a string as the beginning of it IS string data and we can use strstr on it
    // The data after it is binary and cannot be treated as a string
    const char* bodyStart = strstr(request, "\r\n\r\n");

    if (bodyStart)
    {
        bodyStart += strlen("\r\n\r\n");

        /* Because the request we recieve could in theory just have \r\n\r\n at the very end we could read out of bounds
            of the buffer. So we calculate the last byte we will write to and check if it is out of bounds, if so we return FALSE
        */
        if ((requestContentLength > dstMaxSize) || (bodyStart + requestContentLength) > (request + requestBufferSize))
        {
            dbgprintf("Potential out of bounds read detected\n");
            return FALSE;
        }


        errno_t memcpyReturnValue = memcpy_s(message, dstMaxSize, bodyStart, requestContentLength);

        if(memcpyReturnValue)
        {
            dbgprintf("memcpy_s failed with errno %d\n", memcpyReturnValue);

            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

BOOL HlInitListenSocket(SOCKET *pServerSocket, PSOCKADDR_IN pServerAddress, PSOCKADDR_IN pClientAddress)
{

    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        dbgprintf("Error while initializing winsock\n");
        return FALSE;
    }

    // Create socket
    *pServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (*pServerSocket == INVALID_SOCKET)
    {
        dbgprintf("Error: socket failed with WSAError %d\n", WSAGetLastError());

        WSACleanup();

        return FALSE;
    }

    // Set up the sockaddr_in structure
    (*pServerAddress).sin_family = AF_INET;
    (*pServerAddress).sin_addr.s_addr = INADDR_ANY;
    (*pServerAddress).sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(*pServerSocket, (PSOCKADDR)pServerAddress, sizeof(*pServerAddress)) == SOCKET_ERROR)
    {
        dbgprintf("Error: bind failed with WSAError %d\n", WSAGetLastError());

        closesocket(*pServerSocket);
        WSACleanup();

        return FALSE;
    }

    // Listen for incoming connections
    if (listen(*pServerSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        dbgprintf("Error: listen failed with WSAError %d\n", WSAGetLastError());

        closesocket(*pServerSocket);
        WSACleanup();
        
        return FALSE;
    }

    return TRUE;
}

BOOL HlRecieveRequest(SOCKET *pServerSocket, SOCKET *pClientSocket, PSOCKADDR_IN pClientAddress, int ClientAddressLength, _Out_ PBYTE pBuffer, int bufferSize, _Out_ PINT pLength)
{
    *pClientSocket = accept(*pServerSocket, (PSOCKADDR)pClientAddress, &ClientAddressLength);
    if (*pClientSocket == INVALID_SOCKET)
    {

        dbgprintf("Accept failed with error code: %d\n", WSAGetLastError());

        return FALSE;
    }

    // Receive data (why does recv expect a char* ??? wtf microsoft we aint working with strings (well we are a bit but usually you dont))
    int result = recv(*pClientSocket, (char*)pBuffer, bufferSize, 0);

    dbgprintfraw("\n");
    dbgprintf("recieved data! (%d bytes written to buffer)\n", result);

    if (result <= 0)
    {
        dbgprintf("recv failed with error code: %d\n", WSAGetLastError());
        closesocket(*pClientSocket);

        return FALSE;
    }

    *pLength = result;

    return TRUE;
}