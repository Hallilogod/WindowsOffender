#pragma once

#include "../Shared.h"
#include <Windows.h>
#include <conio.h>
#include <stdio.h>

#define INFO(...) printf("[i] " __VA_ARGS__ "\n")
#define WARN(...) printf("[!] " __VA_ARGS__ "\n")
#define QUES(...) printf("[?] " __VA_ARGS__ "\n")
#define ERRO(...) printf("[X] " __VA_ARGS__ "\n")

/*
Unpacks the windows offender binaries to the specified paths in Shared.h

@return TRUE on success, FALSE otherwise.
*/
BOOL InsUnpackImages(void);

/*
Creates all necessary services.

@return TRUE on success, FALSE otherwise. (as always)
*/
BOOL InsCreateServices(SC_HANDLE scmHandle);

/* program entry point (who would have known?) also btw we dont use bloat libc */
void entry(void);


#include "installerUtils.h"