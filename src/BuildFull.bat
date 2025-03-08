@echo off
echo WARNING: The driver needs to be compiled seperately!
cd Server
del Server.exe
cmd /c "Compile"
cd ..
cd DriverLoader
del NativeDriverLoader.exe
cmd /c "CompileLoader"
cd ..
cd Installer
del InstallerResources.res
del WindowsOffenderInstaller.exe
cmd /c "CompileInstallerResources"
cmd /c "CompileInstaller"
move WindowsOffenderInstaller.exe ..