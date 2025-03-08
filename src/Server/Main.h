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


typedef BOOL BOOOL/*if you feel excited*/, BOOOOL/*if you're really excited*/, BOOOOOL/*if you're crazy*/;

#define MAYBE (0.5)

enum BoolEx {
    EX_FALSE,
    EX_TRUE,
    EX_MAYBE,
    EX_LIKELY,
    EX_UNLIKELY,
    EX_PROBABLY_TRUE,
    EX_PROBABLY_FALSE,
    EX_CERTAINLY_FALSE,
    EX_CERTAINLY_TRUE,
    EX_QUANTUM_SUPERPOSITION,
    EX_WHO_KNOWS,              
    EX_I_PREFER_NOT_TO_SAY,    
    EX_ERROR_404_BOOL_NOT_FOUND
};

enum BoolExMaybe {
    EXMAYBE_PROBABLY_NOT,
	EXMAYBE_ALMOST_CERTAINLY_TRUE,
    EXMAYBE_IM_NOT_SURE,
    EXMAYBE_DEFINITELY_MAYBE,
    EXMAYBE_I_HAVE_COMMITMENT_ISSUES
};

// for the Intercal coders
#define PLEASE_DO(x) (x)

// you monster
#define RUST_IS_BETTER_THAN_C (*((char*)0) = 0)


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