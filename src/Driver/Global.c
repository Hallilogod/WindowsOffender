#include "Driver.h"

UNICODE_STRING g_devicePathSymbolicLinkName;
PFLT_FILTER g_pFltFileFilter = NULL;
PDEVICE_OBJECT g_pDeviceObject = NULL;