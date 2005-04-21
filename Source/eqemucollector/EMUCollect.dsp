# Microsoft Developer Studio Project File - Name="EMUCollect" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=EMUCollect - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EMUCollect.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EMUCollect.mak" CFG="EMUCollect - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EMUCollect - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EMUCollect - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EMUCollect - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "C:\IhateWindows\Release"
# PROP Intermediate_Dir "C:\IhateWindows\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /GX /O2 /I "H:\eqemu\src\wpdpack\Include" /I "H:\eqemu\src\zlib\include" /D "PCGUI" /D "COLLECTOR" /D "KEY64" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 packet.lib libz.lib libmySQL.lib wsock32.lib wpcap.lib /nologo /subsystem:windows /machine:I386 /libpath:"H:\eqemu\src\wpdpack\Lib" /libpath:"H:\eqemu\src\zlib\lib"

!ELSEIF  "$(CFG)" == "EMUCollect - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "C:\IhateWindows\Debug"
# PROP Intermediate_Dir "C:\IhateWindows\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /GX /Zi /Ob1 /I "H:\eqemu\src\wpdpack\Include" /I "H:\eqemu\src\zlib\include" /D "PCGUI" /D "COLLECTOR" /D "KEY64" /D "NDEBUG" /D "_CONSOLE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fr /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 packet.lib libz.lib libmySQL.lib wsock32.lib wpcap.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\IhateWindows\Release\EMUCollect.exe" /pdbtype:sept /libpath:"H:\eqemu\src\wpdpack\Lib" /libpath:"H:\eqemu\src\zlib\lib"

!ENDIF 

# Begin Target

# Name "EMUCollect - Win32 Release"
# Name "EMUCollect - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ConfirmDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\dump_packet_header.cpp
# End Source File
# Begin Source File

SOURCE=.\dump_packetfiles.cpp
# End Source File
# Begin Source File

SOURCE=.\dump_pf_privacy.cpp
# End Source File
# Begin Source File

SOURCE=.\EMUCollect.cpp
# End Source File
# Begin Source File

SOURCE=.\EMUCollectDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\eqemucollector.cpp
# End Source File
# Begin Source File

SOURCE=.\EQStreamPair.cpp
# End Source File
# Begin Source File

SOURCE=.\guimain.cpp
# End Source File
# Begin Source File

SOURCE=.\mobinfo_handler.cpp
# End Source File
# Begin Source File

SOURCE=.\MobInfoDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PacketHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\PluginManager.cpp
# End Source File
# Begin Source File

SOURCE=.\PrivacyDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamPairManager.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ConfirmDialog.h
# End Source File
# Begin Source File

SOURCE=.\EMUCollect.h
# End Source File
# Begin Source File

SOURCE=.\EMUCollectDialog.h
# End Source File
# Begin Source File

SOURCE=.\MobInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\PacketCollector.h
# End Source File
# Begin Source File

SOURCE=.\packetfile.h
# End Source File
# Begin Source File

SOURCE=.\PacketHandler.h
# End Source File
# Begin Source File

SOURCE=.\pcdatabase.h
# End Source File
# Begin Source File

SOURCE=.\pcvars.h
# End Source File
# Begin Source File

SOURCE=.\PluginManager.h
# End Source File
# Begin Source File

SOURCE=.\PrivacyDialog.h
# End Source File
# Begin Source File

SOURCE=.\protocol.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SharedLibrary.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StreamPairManager.h
# End Source File
# End Group
# Begin Group "Common Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\classes.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Condition.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CRC16.cpp
# End Source File
# Begin Source File

SOURCE=..\common\crc32.cpp
# End Source File
# Begin Source File

SOURCE=..\common\debug.cpp
# End Source File
# Begin Source File

SOURCE=..\common\emu_opcodes.cpp
# End Source File
# Begin Source File

SOURCE=..\common\EQPacket.cpp
# End Source File
# Begin Source File

SOURCE=..\common\EQStream.cpp
# End Source File
# Begin Source File

SOURCE=..\common\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\misc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MiscFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Mutex.cpp
# End Source File
# Begin Source File

SOURCE=..\common\opcode_map.cpp
# End Source File
# Begin Source File

SOURCE=..\common\opcodemgr.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump_file.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packet_functions.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packetfile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SharedLibrary.cpp
# End Source File
# Begin Source File

SOURCE=..\common\timeoutmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\common\timer.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\EMUCollect.ico
# End Source File
# Begin Source File

SOURCE=.\EMUCollect.rc
# End Source File
# Begin Source File

SOURCE=.\res\EMUCollect.rc2
# End Source File
# End Group
# End Target
# End Project
