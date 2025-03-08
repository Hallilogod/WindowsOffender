#include "Main.h"
#define CHACHA20_IMPLEMENTATION
#include "ChaCha20.h"

UINT32 SecPrngSeed32(void)
{
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    UINT32 rngSeed = (UINT32) ( ((UINT32)sysTime.wSecond << 16)  |
                                ((UINT32)sysTime.wMilliseconds)  );

    return rngSeed;
}


VOID SecChaCha20XorBuffer(PSTR keyString, PBYTE nonce, PBYTE buffer, size_t bufferSize)
{
    BYTE nonceBuffer[12] = { 0 };
    BYTE keyBuffer[32]   = { 0 };

    int keyStringLength = strlen(keyString);
    
    // Copy the key string's characters into the key buffer, 32 bytes max
    memcpy_s(keyBuffer, sizeof(keyBuffer), keyString, keyStringLength <= 32 ? keyStringLength : 32);

    // Copy the nonce into the nonce buffer
    memcpy_s(nonceBuffer, sizeof(nonceBuffer), nonce, 12);

    uint32_t count = 0;

    ChaCha20_Ctx ctx;
	ChaCha20_init(&ctx, keyBuffer, nonceBuffer, count);
	ChaCha20_xor(&ctx, buffer, bufferSize);
}


BOOL SecDecryptMessage(PBYTE rawMessage, size_t encryptedDataLength, size_t maxMessageLength)
{
    BYTE nonce[12] = { 0 };
    BYTE key[32]   = { 0 };
    
    // char* rawMessage = *pRawMessage;

    // Copy the server password into the key buffer, if the password is below 32 bytes the remaining bytes are zero
    // if its bigger then only the first 32 bytes are used
    // the SERVER_PASSWORD length is already length checked (first thing in main()) 
    memcpy_s(key, sizeof(key), SERVER_PASSWORD, strlen(SERVER_PASSWORD));

    // if the message is only 12 bytes or smaller then this doesnt make sense
    if(encryptedDataLength <= 12)
    {
        dbgprintf("Encrypted raw message is too small\n");
        return FALSE;
    }
    
    if(encryptedDataLength >= (maxMessageLength - 12))
    {
        dbgprintf("Encrypted raw message is too big (%d)\n", encryptedDataLength);

        return FALSE;
    }

    // Copy the nonce into the nonce buffer
    memcpy_s(nonce, sizeof(nonce), rawMessage, 12);

    // Increment the pointer by 12 to skip the nonce and start decrypting
    rawMessage += 12;

    int count = 0;

    ChaCha20_Ctx ctx;
	ChaCha20_init(&ctx, key, nonce, count);
	ChaCha20_xor(&ctx, rawMessage, encryptedDataLength - 12);
    
    return TRUE;
}


BOOL SecValidateRequestPasswordHash(char* message, _Out_ int* messageStartIndex)
{
    BOOL returnValue = FALSE;

    int HashLen = 0;    

    // Find the position of the first colon, as message is actually one null byte bigger than buffer size, we wont read out of it
    for(size_t i = 0; i < strlen(message); i++)
    {
        if(message[i] == ':')
        {
            // Found it? then store this position in HashLen.
            HashLen = i;
            break;
        }
    }
    
    if(0 == HashLen)
    {
        return returnValue;
    }

    // Callocate some memory for the hash to check (calloc for null terminator go brr)
    char* hashToCheck = calloc(HashLen + 1 /*nullterminator*/, sizeof(char));

    if(hashToCheck == NULL)
    {
        dbgprintf("Failed to allocate memory for hash");

        return returnValue;
    }
    
    memcpy_s(hashToCheck, (HashLen + 1) * sizeof(char), message, HashLen);

    char passwordHash[SHA256_HEX_SIZE];

    // Compute SHA256 sum
    sha256_hex(SERVER_PASSWORD, strlen(SERVER_PASSWORD), passwordHash);

    *messageStartIndex = HashLen + 1;

    if(streql(passwordHash, hashToCheck))
    {   
        returnValue = TRUE;
    }

    free(hashToCheck);

    return returnValue;
   
}

