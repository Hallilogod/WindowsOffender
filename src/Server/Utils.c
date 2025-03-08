#include "Main.h"

BOOL UtAddPrivilegeToOwnToken(LPSTR PrivilegeName)
{
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueA(NULL, PrivilegeName, &tp.Privileges[0].Luid))
    {

        return FALSE;
    }

    HANDLE ownProcess;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &ownProcess))
    {
        return FALSE;
    }
    if (!AdjustTokenPrivileges(ownProcess, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
    {
        CloseHandle(ownProcess);
        return FALSE;
    }
    if (GetLastError() != ERROR_SUCCESS)
    {
        CloseHandle(ownProcess);
        return FALSE;
    }
    CloseHandle(ownProcess);
    return TRUE;
}


BOOL UtTwelveDigitNumber(ULONGLONG number,_Out_ PCHAR pBuffer)
{

    if(!pBuffer)
    {
        return FALSE;
    }

    int digitsCounter = 0;
    ULONGLONG numberCopy = number;
    do
    {
        digitsCounter++;
        numberCopy /= 10;
    } while (numberCopy > 0);

    while(digitsCounter > 12)
    {
        number /= 10;
        digitsCounter--;
    }

    if(sprintf_s(pBuffer,
               13,
               "%012llu",
               number) != 12)
    {
        return FALSE;
    }

    return TRUE;
}


UINT UtCountChar(const char *str, char charToCount)
{
    UINT count = 0;

    while (*str != '\0')
    {
        if (*str == charToCount)
        {
            count++;
        }

        str++;
    }

    return count;
}

ULONGLONG UtCountDigits(ULONGLONG number)
{
    ULONGLONG count = 0;

    do {
        count++;
        number /= 10;
    } while (number > 0);

    return count;
}