# Microsoft Developer Studio Project File - Name="voodoo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=voodoo - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "voodoo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "voodoo.mak" CFG="voodoo - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "voodoo - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "voodoo - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "voodoo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\lib\kclient" /I "..\..\lib\krb" /I "..\..\lib\des" /I "..\..\include" /I "..\..\include\win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\..\lib\kclient\Release\kclnt32.lib ..\..\lib\krb\Release\krb.lib ..\..\lib\des\Release\des.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "voodoo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I "..\..\lib\kclient" /I "..\..\lib\krb" /I "..\..\lib\des" /I "..\..\include" /I "..\..\include\win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ..\..\lib\kclient\Debug\kclnt32.lib ..\..\lib\krb\Debug\krb.lib ..\..\lib\des\Debug\des.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "voodoo - Win32 Release"
# Name "voodoo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\AuthOption.cpp
# End Source File
# Begin Source File

SOURCE=.\CharStream.cpp
# End Source File
# Begin Source File

SOURCE=.\CryptoEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\DenyAllOption.cpp
# End Source File
# Begin Source File

SOURCE=.\EmulatorEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\EncryptOption.cpp
# End Source File
# Begin Source File

SOURCE=.\Negotiator.cpp
# End Source File
# Begin Source File

SOURCE=.\Option.cpp
# End Source File
# Begin Source File

SOURCE=.\TelnetApp.cpp
# End Source File
# Begin Source File

SOURCE=.\TelnetEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\TelnetResource.rc
# End Source File
# Begin Source File

SOURCE=.\TelnetSession.cpp
# End Source File
# Begin Source File

SOURCE=.\TerminalEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\WinSizeOption.cpp
# End Source File
# Begin Source File

SOURCE=.\YesNoOptions.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\AuthOption.h
# End Source File
# Begin Source File

SOURCE=.\char_codes.h
# End Source File
# Begin Source File

SOURCE=.\CharStream.h
# End Source File
# Begin Source File

SOURCE=.\CryptoEngine.h
# End Source File
# Begin Source File

SOURCE=.\DenyAllOption.h
# End Source File
# Begin Source File

SOURCE=.\EmulatorEngine.h
# End Source File
# Begin Source File

SOURCE=.\EncryptOption.h
# End Source File
# Begin Source File

SOURCE=.\Negotiator.h
# End Source File
# Begin Source File

SOURCE=.\Option.h
# End Source File
# Begin Source File

SOURCE=.\Telnet.h
# End Source File
# Begin Source File

SOURCE=.\TelnetApp.h
# End Source File
# Begin Source File

SOURCE=.\TelnetCodes.h
# End Source File
# Begin Source File

SOURCE=.\TelnetEngine.h
# End Source File
# Begin Source File

SOURCE=.\TelnetSession.h
# End Source File
# Begin Source File

SOURCE=.\TerminalEngine.h
# End Source File
# Begin Source File

SOURCE=.\WinSizeOption.h
# End Source File
# Begin Source File

SOURCE=.\YesNoOptions.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
