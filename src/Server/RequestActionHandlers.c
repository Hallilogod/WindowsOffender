#include "Main.h"

void RaFreeThreadedActionParameters(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
    while((pActionFunctionParameters->ArgumentCount)--)
    {
        free((pActionFunctionParameters->Arguments)[pActionFunctionParameters->ArgumentCount]);
    }

    free(pActionFunctionParameters->Arguments);

    free(pActionFunctionParameters);
}


SERVER_STATUS_CODE RaPowerControl(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
    char** Arguments = pActionFunctionParameters->Arguments;

    int powerOption = -1;

    if(streql(Arguments[2], "poweroff"))
        {
            powerOption = 0;
        }
        else if (streql(Arguments[2], "reboot"))
        {
            powerOption = 1;
        }
        else if(streql(Arguments[2], "halt"))
        {
            powerOption = 2;
        }


    // we need SeShutdown priv
    if(streql(Arguments[1], "normal"))
    {
        UINT Flags = EWX_FORCE;

        if(powerOption == 0)
        {
            Flags |= EWX_POWEROFF;
        }
        else if(powerOption == 1)
        {
            Flags |= EWX_REBOOT;
        }
        else
        {

            dbgprintf("Error RaPowerControl: invalid power option %s for normal power control", Arguments[1]);

            return SERVER_STATUS_INVALID_ARGUMENTS;
        }

        if(!ExitWindowsEx(Flags, SHTDN_REASON_MAJOR_OTHER))
        {
            dbgprintf("ExitWidowsEx failed with lasterror %lu\n", GetLastError());
            return SERVER_STATUS_UNSUCCESSFUL;
        }

        return SERVER_STATUS_SUCCESS;
    }
    // we need SeShutdown priv too
    else if(streql(Arguments[1], "native"))
    {
        if(powerOption != 0 && powerOption != 1)
        {
            dbgprintf("Invalid power option %s for native shutdown\n", Arguments[1]);
            
            return SERVER_STATUS_INVALID_ARGUMENTS;
        }

        if(!NT_SUCCESS(NtShutdownSystem(powerOption)))
        {
            dbgprintf("NtShutdownSystem failed, SeShutdownPrivilege was probably not enabled correctly\n");
            return SERVER_STATUS_UNSUCCESSFUL;
        }
    }

    else if(streql(Arguments[1], "firmware"))
    {

        int firmwareReentry = (powerOption == 2) ? HalHaltRoutine : ((powerOption == 1) ? HalRebootRoutine : ((powerOption == 0) ? HalPowerDownRoutine : -1));

        if(firmwareReentry == -1)
        {
            dbgprintf("Invalid power option %s for firmware shutdown\n", Arguments[2]);

            return SERVER_STATUS_INVALID_ARGUMENTS;
        }

        return SendIOCTL(IOCTL_FIRMWAREPOWER, &firmwareReentry, sizeof(FIRMWARE_REENTRY), NULL, 0, NULL);

    }
    else if(streql(Arguments[1], "triplefault"))
    {
        if(powerOption != 1)
        {
            dbgprintf("Triple fault only works with power option reboot\n");
            return SERVER_STATUS_INVALID_ARGUMENTS;
        }

        return SendIOCTL(IOCTL_TRIPLE_FAULT, NULL, 0, NULL, 0, NULL);
    }
    else
    {
        return SERVER_STATUS_INVALID_ARGUMENTS;
    }
}

SERVER_STATUS_CODE RaTerminateProcess(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
    char** Arguments = pActionFunctionParameters->Arguments;

    ULONG pidToTerminate = strtoul(Arguments[1], NULL, 10);

    if(0 == pidToTerminate)
    {
       
        dbgprintf("Invalid PID: %s\n", Arguments[1]);
        
        return SERVER_STATUS_INVALID_ARGUMENTS;

    }

    return SendIOCTL(IOCTL_TERMINATEPROCESS, &pidToTerminate, sizeof(ULONG), NULL, 0, NULL);
}

SERVER_STATUS_CODE RaThreadedMessageBox(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
    char** Arguments = pActionFunctionParameters->Arguments;

    MessageBoxA(NULL, Arguments[1], Arguments[2], (UINT)strtoul(Arguments[3], NULL, 0/*auto determine base*/));


    RaFreeThreadedActionParameters(pActionFunctionParameters);

    return SERVER_STATUS_SUCCESS;
}



SERVER_STATUS_CODE RaPing(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
   PACTION_RESPONSE pActionResponse = pActionFunctionParameters->pActionResponse;

   pActionResponse->outputBuffer = "Pong!";
   pActionResponse->outputBufferSize = strlen("Pong!");

    return SERVER_STATUS_SUCCESS;
}



SERVER_STATUS_CODE RaSelfDestroy(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
    return SendIOCTL(IOCTL_SELFDESTROY, NULL, 0, NULL, 0, NULL);
}


// For debugging purposes
#ifdef DEBUG
SERVER_STATUS_CODE RaDebugTest(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters)
{
   
   return SERVER_STATUS_SUCCESS;
}
#endif