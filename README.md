# Welcome to WindowsOffender

WindowsOffender is a management software and rootkit for Windows managable over a [Web Panel](https://github.com/a-catgirl-dev/windows-offender-panel) and is the largest project i ever made for now. All components of it are written in C, except for the panel, which is written by [a catgirl](https://github.com/a-catgirl-dev) because i can't do web dev, though she [also struggles with it a lot](https://github.com/a-catgirl-dev/windows-offender-panel/commit/f61854c597aed2c7b1f91b7a1f5bfdaca474e7a1).

# What this is

WindowsOffender is not a typical rootkit. It doesn't have completely bulletproof self-hiding mechanisms and can easily be detected by good AV engines (although i have never tested it with something else than Windows Defender). The goal for this project was never to be a completely perfect rootkit, because with UEFI Secure Boot and modern rootkit mitigation mechanisms it's not as easy anymore to infect the early boot chain of a system and be invisible to the entire operating system. The goal of WindowsOffender was to have a rootkit / management software that works on modern windows systems while also bypassing Windows Defender (hence the name) and also being capable of self-hiding mechanisms. These self-hiding mechanisms aren't perfect and only work when the driver is running, if the system is not running the driver and driver loader are laying on disk passively. This rootkit is not suitable for things like botnets (unless you do heavy modifications to it) but rather for targeting specific systems and backdooring these. In fact it comes with a simple CLI installer that you need to run elevated on the target system, so its useful if you have gained physical access to a device once that you want to backdoor.

# Important current limitations

**Help needed for Windows 11:**
WindowsOffender currently does not work on Windows 11 because the pattern search of the kernel's validation routines that the driver loader patches fails most times on win11 and i wasn't able to figure out why this happens. You are very very welcomed to look into this issue and figure out why it doesn't work. The pattern itself isn't different in the windows 11 kernel and the locating does in fact sometimes work, its very confusing. Take a peek at the SearchForPatternPhysical calls in [DriverLoader.c](src/DriverLoader/DriverLoader.c). I'd really appreaciate any help!

- WindowsOffender currently requires you to know know the ip address of the machine you installed it on because the server simply listens on a port for requests.
- Because the driver loader is executed by the Session Manager via the SetupExecute registry value you should make sure that the system you install WindowsOffender on do not use this key for anything else important (which is basically never the case) as it currently only writes itself into that value and deletes the value when uninstalled.
- The current version does not hide the server process

# Planned future features

- Automatic firewall rule adding option in the installer
- Robust SetupExecute adding / removing mechanism in case the value already contains something
- Driver and server binary runtime extracting from the driverloader to only have a single file on disk
- Hiding of the SetupExecute driver loader entry
- Process hiding mechanism
- An option to make the server send something to a specified address so you can know the ip address of the system, this might also replace the server listening in the future because it can establish a connection from the system and thus bypass firewalls
- All kinds of features you would expect from a rootkit (eg keylogger, file system control, more process options etc)

# Building

To build WindowsOffender, you need Visual Studio with the [Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) for building the driver and [mingw](https://github.com/niXman/mingw-builds-binaries/releases) to compile the server, driver loader and installer. Before building anything you need to make a few configurations in [Shared.h](src/Shared.h):

- SERVER_PASSWORD: The password that will be used to encrypt the communcation between the panel and server
- SERVER_PORT: The port that the server will listen on
- HIDEMYASS: Whether you want to enable the self hiding mechanisms
- USE_DEFAULT_NAMES: Whether WindowsOffender should name its components by the default names (for non-hidden installations)
- Change the non-default names of the components when not using USE_DEFAULT_NAMES

The components of WindowsOffender are in their respective folders, to build a full installer you first need to build the driver. Open the WindowsOffenderDriver solution and compile a x64 Release build. Next go to the src directory and execute the BuildFull.bat script. If everything goes well, you should get a the installer named WindowsOffenderInstaller.exe

# How to use

After installing and rebooting WindowOffender on a target machine, you just need to put the ip address of the system, the port the server listens on, and the password into the web panel's respective fields, and you can start commanding it. Whether you can talk to the system over a network is up to you.

# Architecture

WindowsOffender consists of the driver loader, the kernel driver, the server and the web panel. The driver loader is a native app which is loaded at system boot via SetupExecute and starts the driver. It does this by abusing vulnerable signed drivers to gain R/W access to kernel memory and patch validation routines so that the WindowsOffender driver can be started. The Driver is responsible for doing the low-level/privileged operations and the filtering. The server is the usermode app responsible for listening on the specified port, recieving requests, validating them, handling them, communicating with the driver and sending a response to the panel. The panel is responsible for sending the user specified requests in the correct format and showing the server response. For a full architectual overview see [src](src/)

Basic architectual diagram of WindowsOffender on a running system:

![WindowsOffender architectual diagram](Assets/WindowsOffender%20Architecture.png)

# A few more words

It should be very clear that this project is based on a lot of native, undocumented and unintended stuff. Im trying my best to make this project as robust as possible, but you can never count on it working to 100%. Nobody is responsible for what you do with this, feel free to ask us any questions.
