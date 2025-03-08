#include "Main.h"

// Array that associates the request strings with their corresponding RequestAction function
REQUEST_ACTION RequestActionList[] =

    {
#ifdef DEBUG
        {"debugtest", RaDebugTest, 0, FALSE},
#endif

        {"powercontrol", RaPowerControl, 2, FALSE},
        {"terminateprocess", RaTerminateProcess, 1, FALSE},
        {"messagebox", RaThreadedMessageBox, 3, TRUE},
        {"ping", RaPing, 0, FALSE},
        {"selfdestroy", RaSelfDestroy, 0, FALSE}

};


SERVER_STATUS_CODE RhHandleNetworkRequest(char *request, _Out_ PACTION_RESPONSE pActionResponse)
{

    SERVER_STATUS_CODE statusResponse = SERVER_STATUS_SUCCESS;

    size_t requestLen = strlen(request) + 1;

    // request must be present and may not end with a colon
    if (requestLen < 2 || request[requestLen - 2] == ':')
    {
        return SERVER_STATUS_INVALID_REQUEST_SYNTAX;
    }

    // Don't forget to free
    char *requestCopy = malloc(requestLen);

    if(requestCopy == NULL)
    {
        dbgprintf("Failed to allocate memory for request copy");

        return SERVER_STATUS_REQUEST_HANDLING_ERROR;
    }

    strcpy_s(requestCopy, requestLen, request);

    UINT argumentCount = UtCountChar(requestCopy, ':') + 1;

    // Array, free aswell!
    char **Arguments = malloc(argumentCount * sizeof(char *));

    if(Arguments == NULL)
    {
        dbgprintf("Failed to allocate memory for argument array");

        free(requestCopy);

        return SERVER_STATUS_REQUEST_HANDLING_ERROR;
    }

    char *token = strtok(requestCopy, ":");

    int argumentCounter = 0;

    while (token != NULL)
    {
        Arguments[argumentCounter] = malloc(requestLen /*prob too much but we avoid too small buffer*/);

        if(Arguments[argumentCounter] == NULL)
        {
            dbgprintf("Failed to allocate memory for arguments");

            for(int i = 0; i < argumentCounter; i++)
            {
                free(Arguments[i]);
            }
            
            free(requestCopy);
            free(Arguments);

            return SERVER_STATUS_REQUEST_HANDLING_ERROR;
        }

        strcpy_s(Arguments[argumentCounter], requestLen, token);
        token = strtok(NULL, ":");
        argumentCounter++;
    }

    free(requestCopy);

    for (size_t i = 0; i < _countof(RequestActionList); i++)
    {
        if (streql(Arguments[0], RequestActionList[i].RequestID))
        {
            BOOL threadCreated = FALSE;
            do
            {
                // Check if there aren't enough arguments provided
                if (argumentCounter > 0 && (argumentCounter - 1) < RequestActionList[i].MinimumArgumentCount)
                {
                    statusResponse = SERVER_STATUS_NOT_ENOUGH_ARGUMENTS;

                    break;
                }

                if(RequestActionList[i].RunThreaded)
                {
                    DWORD threadId = 0;
                    // This also has to get freed by the thread
                    PACTION_FUNCTION_PARAMETERS pThreadedActionFunctionParameters = malloc(sizeof(ACTION_FUNCTION_PARAMETERS)); 

                    pThreadedActionFunctionParameters->Arguments = Arguments;
                    pThreadedActionFunctionParameters->ArgumentCount = argumentCounter;
                    // Threaded actions can neither return a server status nor data
                    pThreadedActionFunctionParameters->pActionResponse = NULL;


                    HANDLE actionFunctionThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RequestActionList[i].ActionFunction, pThreadedActionFunctionParameters, 0, &threadId);

                    if(actionFunctionThread == NULL)
                    {
                        dbgprintf("Failed to create action function thread");

                        free(pThreadedActionFunctionParameters);

                        statusResponse = SERVER_STATUS_THREAD_CREATION_FAILED;

                        break;
                    }

                    threadCreated = TRUE;

                    CloseHandle(actionFunctionThread);
                    
                    size_t threadIdBufferSize = UtCountDigits(threadId) + 1/*null terminator*/;

                    char* threadIdStringHeapBuffer = calloc(1, threadIdBufferSize);

                    // Catch memory allocation errors
                    if(threadIdStringHeapBuffer == NULL)
                    {
                        dbgprintf("Failed to allocate thread id string buffer");

                        statusResponse = SERVER_STATUS_REQUEST_HANDLING_ERROR;

                        break;
                    }

                    // Important! otherwise we leak memory
                    pActionResponse->bufferHeapAllocated = TRUE;
                    pActionResponse->outputBuffer = threadIdStringHeapBuffer;
                    pActionResponse->outputBufferSize = threadIdBufferSize;

                    // If this fails we just return Unknown as the thread id
                    if(sprintf_s(threadIdStringHeapBuffer, threadIdBufferSize, "%lu", threadId) < 0)
                    {
                        dbgprintf("Error while converting thread id to string format");

                        free(threadIdStringHeapBuffer);

                        pActionResponse->bufferHeapAllocated = FALSE;
                        pActionResponse->outputBuffer = "Unknown";
                        pActionResponse->outputBufferSize = strlen("Unknown") + 1;
                    }

                    statusResponse = SERVER_STATUS_THREADED_ACTION;
                }
                else
                {
                    ACTION_FUNCTION_PARAMETERS actionFunctionParameters = { Arguments, argumentCounter, pActionResponse};

                    // Call the corresponding request handling function
                    statusResponse = RequestActionList[i].ActionFunction(&actionFunctionParameters);
                }
            

            } while(FALSE);
            
            /* For now threaded request just plainly have to manually free all arguments they recieved and the argument array, 
                i hope we can find a better way for this in the future
            */
            if(!threadCreated)
            {
                for (int i = 0; i < argumentCounter; i++)
                {
                    free(Arguments[i]);
                }

                free(Arguments);
            }


            return statusResponse;
        }
    }

    for (int i = 0; i < argumentCounter; i++)
    {
        free(Arguments[i]);
    }

    free(Arguments);

    return SERVER_STATUS_UNKNOWN_REQUEST;
}

size_t RhParseRequestContentLength(char *httpRequest, size_t requestBufferSize)
{
    int contentLength = 0;

    const char *contentLengthHeader = "Content-Length: ";
    char *start = strstr(httpRequest, contentLengthHeader);
    if (start)
    {

        start += strlen(contentLengthHeader);

        int digitCounter = 0;

        while (isdigit(start[digitCounter]) && (start + digitCounter) < (httpRequest + requestBufferSize))
        {
            digitCounter++;

            // If this is true then what the fuck massive bloat did we recieve, we will not tolerate this
            if (digitCounter > 4)
            {
                dbgprintf("DigitCounter exceeded 4, returning zero for content length\n");
                return 0ULL;
            }
        }

        /*
        Null the byte after the digits, use atoi and restore it, i prefer this over a reverse convert contentLength += (((*--start) - '0') * pow(10, counter));
        (notice how we have come to a point where i alr implemented my own atoi for a fucking request parsing)
        */
        char temp = start[digitCounter];

        start[digitCounter] = '\0';

        contentLength = atoi(start);

        start[digitCounter] = temp;
    }
    else
    {
        dbgprintf("Content-Length header not found\n");
    }

    if(contentLength < 1)
    {
        return 0ULL;
    }

    return (size_t)contentLength;
}

BOOL RhRespondToRequestEncrypted(SOCKET clientSocket, SERVER_STATUS_CODE requestStatusCode, PACTION_RESPONSE pActionResponse, ULONGLONG encryptionNonce)
{

    BOOL additionalData = FALSE;

    CHAR responseNonceString[12 + 1] = {0}; /*12 for the characters, 1 for the null terminator*/

    UINT nonceLength = 0;
    const ULONGLONG statusCodeLength = 4;

    // Check if we have additional data, if not then we shouldn't append a nonce
    if (pActionResponse->outputBufferSize > 0)
    {
        if (!UtTwelveDigitNumber(encryptionNonce, responseNonceString))
        {
            dbgprintf("Failed to convert encryption nonce (%llu) to 12-digit number\n", encryptionNonce);

            return FALSE;
        }

        nonceLength = 12;
        additionalData = TRUE;
    }

    PCHAR httpHeader;

    if (additionalData)
    {
        httpHeader =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %llu\r\n"
            "Access-Control-Allow-Origin: *\r\n"             // Allow requests from any origin
            "Access-Control-Allow-Methods: GET, POST\r\n"    // Allow specific methods
            "Access-Control-Allow-Headers: Content-Type\r\n" // Allow specific headers
            "Connection: close\r\n\r\n"
            "%04lu" // the status code, the first 4 characters are always the status code
            "%s"    // The 12 character nonce
            "%s";   // the additional data
    }
    else
    {
        httpHeader =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %llu\r\n"
            "Access-Control-Allow-Origin: *\r\n"             // Allow requests from any origin
            "Access-Control-Allow-Methods: GET, POST\r\n"    // Allow specific methods
            "Access-Control-Allow-Headers: Content-Type\r\n" // Allow specific headers
            "Connection: close\r\n\r\n"
            "%04lu"; // the status code, the first 4 characters in the content are always the status code
    }

    CHAR rawHttpHeader[] = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: \r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: GET, POST\r\n"
                           "Access-Control-Allow-Headers: Content-Type\r\n"
                           "Connection: close\r\n\r\n";

    size_t rawHttpHeaderLength = strlen(rawHttpHeader);

    ULONGLONG contentLength;

    if (additionalData)
    {
        // status code + nonce + additional data
        contentLength = statusCodeLength + nonceLength + pActionResponse->outputBufferSize;
    }
    else
    {
        // status code
        contentLength = statusCodeLength;
    }

    ULONGLONG contentLengthDigitCount = UtCountDigits(contentLength);

    // http header length + content length number digit length + content length + null terminator
    size_t bytesToAlloc = rawHttpHeaderLength + contentLengthDigitCount + contentLength + 1;

    PCHAR httpResponse = malloc(bytesToAlloc);

    if (httpResponse == NULL)
    {
        return FALSE;
    }

    if (additionalData)
    {
        _snprintf_s(
            httpResponse, // buffer to copy to
            bytesToAlloc, // size of buffer
            bytesToAlloc, // max chars to write
            httpHeader,   // http header format string

            contentLength,                  // Content length
            requestStatusCode,              // The 4-character status code
            responseNonceString,            // The 12-character nonce
            pActionResponse->outputBuffer); // The additional data
    }
    else // No additional data
    {
        _snprintf_s(
            httpResponse, // buffer to copy to
            bytesToAlloc, // size of buffer
            bytesToAlloc, // max chars to write
            httpHeader,   // http header format string

            statusCodeLength,   // Content length, 4 for status code
            requestStatusCode); /*The status code*/
    }

    if (additionalData)
    {
        // Create a pointer to the first byte of the additional data in the response
        PBYTE additionalDataPointer = (PBYTE)(httpResponse + rawHttpHeaderLength + contentLengthDigitCount + statusCodeLength + nonceLength);

        // Encrypt the additional data
        SecChaCha20XorBuffer(SERVER_PASSWORD, (PBYTE)responseNonceString, additionalDataPointer, pActionResponse->outputBufferSize);
    }

    // Finally, send the response :3 // the :3 is for a hide and seek for william i do NOT use ":3" under any circumstances except for this
    INT bytesSent = send(clientSocket, httpResponse, (bytesToAlloc - 1), 0);

    free(httpResponse);

    dbgprintf("Response bytes sent: %d\n", bytesSent);

    return TRUE;
}
