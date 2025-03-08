// blame me for having a Utils file if you want, idc
#pragma once


/*
@brief Enables a Privilege in the own process token.

@param PrivilegeName: name of the Privilege to enable for the own token.

@return TRUE on success, FALSE otherwise.
*/
BOOL UtAddPrivilegeToOwnToken(LPSTR PrivilegeName);



/*
@brief Converts an unsigned long long to a 12 character long string, too big values will get truncated,
too small numbers will get padded with zeroes.

@param number the number to convert.
@param pBuffer a pointer to a 13 character big buffer (12 for the number 1 for null terminator)

@return TRUE on success, FALSE otherwise.
*/
BOOL UtTwelveDigitNumber(ULONGLONG number, PCHAR pBuffer);


/*
@brief Counts the digits of an unsigned long long in decimal format.

@param number the number to count the digits of in decimal base.

@return the amount of digits.
*/
ULONGLONG UtCountDigits(ULONGLONG number);


/*
@brief Counts the occurences of charToCount in str.

@param str the string to search for charToCount in.
@param charToCount the character to search in str.

@return the amount of occurences of charToCount in str.
*/
UINT UtCountChar(const char *str, char charToCount);