#pragma once
/*
RequestHandler:

- Handles the request recieved from client and calls the corresponding RequestAction.
*/




typedef struct _ACTION_RESPONSE
{
    // The size of the output buffer in bytes, if 0, there is none.
    ULONGLONG outputBufferSize;

    // Output Buffer
    char* outputBuffer;

    /* Indicates whether the output buffer was malloc'ed and needs to be freed,
        if so the server will automatically free the output buffer after responding
        to the request it recieved and gives action functions an easy way to return
        arbitrary sized data.
    */
    BOOL bufferHeapAllocated; 

} ACTION_RESPONSE, *PACTION_RESPONSE;

/*
Parameter struct passed to action functions.
*/
typedef struct _ACTION_FUNCTION_PARAMETERS
{   
    // String array of arguments, first Argument is the action function ID.
    char** Arguments;

    // Amount of arguments, including the action function ID argument.
    int ArgumentCount;

    // Action response.
    PACTION_RESPONSE pActionResponse;

} ACTION_FUNCTION_PARAMETERS, *PACTION_FUNCTION_PARAMETERS;


typedef SERVER_STATUS_CODE (*ActionFunction)(PACTION_FUNCTION_PARAMETERS);

// Represents a request from a client.
typedef struct _REQUEST_ACTION
{
    // Request ID.
    LPSTR RequestID; 

    // Function responsible for doing the request specific work.
    ActionFunction ActionFunction; 

    // Minimum amount of arguments, not including the first ID argument.
    int MinimumArgumentCount;

    // Whether the request action function should be called in a new thread. Threaded action functions need to manually free the arguments
    BOOL RunThreaded;

} REQUEST_ACTION, *PREQUEST_ACTION;




/*
@brief handles a request after it has been fully validated, the only parsing to do in this function is the argument and function name seperating,
the function will then call the appropriate request handling function.

@param request the string of the actual request without the hash, "request:arg1:arg2...".

@param pActionRespone a pointer to an ACTION_RESPONSE struct that will recieve the appropriate data (if any).

@return the appropriate SERVER_STATUS_CODE.
*/
SERVER_STATUS_CODE RhHandleNetworkRequest(char* request, _Out_ PACTION_RESPONSE pActionResponse);


/*
@brief extracts the value of the Content-Length header in the recieved HTTP request.

@param httpRequest: The raw HTTP request recieved from the client.

@return the Content-Length value as an integer, or 0 on failure (or if the content-length was 0 but that is a problem too so treat 0 as error).
*/
size_t RhParseRequestContentLength(char *httpRequest, size_t requestBufferSize);


BOOL RhRespondToRequestEncrypted(SOCKET clientSocket, DWORD requestStatusCode, PACTION_RESPONSE pActionResponse, ULONGLONG encryptionNonce);