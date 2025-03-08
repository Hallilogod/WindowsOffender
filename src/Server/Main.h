#pragma once

#include "../Shared.h"

#include <winsock2.h>
#include <Windows.h>
#include <winternl.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#define streql(string1, string2) (!strcmp((string1), (string2)))

#define STRINGIFY_INTERNL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNL(x)


#ifdef DEBUG
#define dbgprintf(...) printf("[" __FILE__ ":" STRINGIFY(__LINE__) "] " __VA_ARGS__)
#define dbgprintfraw printf
#else
#define dbgprintf  
#define dbgprintfraw  
#endif


typedef enum _FIRMWARE_REENTRY
{

	HalHaltRoutine,
	HalPowerDownRoutine,
	HalRestartRoutine,
	HalRebootRoutine,
	HalInteractiveModeRoutine,
	HalMaximumRoutine

} FIRMWARE_REENTRY, * PFIRMWARE_REENTRY;

typedef enum _SHUTDOWN_ACTION
{

    ShutdownNoReboot, 
    ShutdownReboot,
    ShutdownPowerOff

} SHUTDOWN_ACTION, *PSHUTDOWN_ACTION;

NTSTATUS NtShutdownSystem(SHUTDOWN_ACTION Action);


#include "ServerStatusCodes.h"
#include "sha256.h"
#include "Utils.h"
#include "Security.h"
#include "HttpListen.h"
#include "DriverCommunication.h"
#include "RequestHandler.h"
#include "RequestActionHandlers.h"

int main(void);