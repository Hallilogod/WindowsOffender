#pragma once

/*
Security.h

Contains stuff related to the cryptography and security in WindowsOffender
*/

#define NONCE_LENGTH_BYTES 12

/*
@brief read below

@return a PSEUDO-RNG unsigned 32 bit number based on the system's time.

*/
UINT32 SecPrngSeed32(void);

/*
@brief Validates the password hash of a recieved request and tells the start position of the actual request.

@param message: The (encrypted) and HTTP parsed message recieved from the client.
@param messageStartIndex: Pointer to an integer that will recieve the index of the message where the actual message starts.

@return TRUE if the Hash matches the password hash, FALSE otherwise.
*/
BOOL SecValidateRequestPasswordHash(char* message, _Out_ int* messageStartIndex);


/*
@brief Decrypts the recieved message from the client, it modifies rawMessage directly

@param rawMessage: the raw message recieved from the client (already HTTP parsed by HlParseRequestMsg)
                it is expected to have a 12 byte long nonce prepended and be null terminated properly.

@param encryptedDataLength: exact byte size of the encrypted data

@param maxMessageLength maximum size of rawMessage in bytes, must not equal the size of the actual string itself and is only used to avoid potential memory corruption.

@return TRUE if decryption was successful, FALSE otherwise.
*/
BOOL SecDecryptMessage(PBYTE rawMessage, size_t encryptedDataLength, size_t maxMessageLength);



/*
@brief Xors a buffer with ChaCha20 

@param string key to xor buffer with, if less than 32 characters, null bytes will be appended, will read 32 characters max
@param nonce pointer to a 12 byte long nonce
@param buffer buffer to xor
@param bufferSize size of buffer in bytes

@return nothing

*/
VOID SecChaCha20XorBuffer(PSTR key, PBYTE nonce, PBYTE buffer, size_t bufferSize);
