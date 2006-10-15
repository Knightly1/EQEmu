# Microsoft Developer Studio Project File - Name="EQBuilder" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=EQBuilder - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EQBuilder.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EQBuilder.mak" CFG="EQBuilder - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EQBuilder - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EQBuilder - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EQBuilder - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "c:\mysql\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "INVERSEXY" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 libmysql.lib /nologo /subsystem:windows /machine:I386 /libpath:"c:\mysql\lib\debug"

!ELSEIF  "$(CFG)" == "EQBuilder - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "C:\mysql\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "INVERSEXY" /FAs /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libmysql.lib /nologo /subsystem:windows /profile /debug /machine:I386 /libpath:"c:\mysql\lib\debug"

!ENDIF 

# Begin Target

# Name "EQBuilder - Win32 Release"
# Name "EQBuilder - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BigZoneDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\common\buildfile.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildFileLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildLogic.cpp
# End Source File
# Begin Source File

SOURCE=.\cdoor.cpp
# End Source File
# Begin Source File

SOURCE=.\cgrid.cpp
# End Source File
# Begin Source File

SOURCE=.\ckill.cpp
# End Source File
# Begin Source File

SOURCE=.\cloc.cpp
# End Source File
# Begin Source File

SOURCE=.\clog.cpp
# End Source File
# Begin Source File

SOURCE=.\clustering.cpp
# End Source File
# Begin Source File

SOURCE=.\cmerchant.cpp
# End Source File
# Begin Source File

SOURCE=.\cmob.cpp
# End Source File
# Begin Source File

SOURCE=.\cnpc.cpp
# End Source File
# Begin Source File

SOURCE=.\combining.cpp
# End Source File
# Begin Source File

SOURCE=.\CPathSplit.cpp
# End Source File
# Begin Source File

SOURCE=.\cspawn.cpp
# End Source File
# Begin Source File

SOURCE=.\cteleport.cpp
# End Source File
# Begin Source File

SOURCE=.\cwaypoint.cpp
# End Source File
# Begin Source File

SOURCE=.\czone.cpp
# End Source File
# Begin Source File

SOURCE=.\database.cpp
# End Source File
# Begin Source File

SOURCE=.\DBConnect.cpp
# End Source File
# Begin Source File

SOURCE=.\door_list.cpp
# End Source File
# Begin Source File

SOURCE=.\EQBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\EQBuilder.rc
# End Source File
# Begin Source File

SOURCE=.\EQBuilderDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\eqbuilderini.cpp
# End Source File
# Begin Source File

SOURCE=.\grid_list.cpp
# End Source File
# Begin Source File

SOURCE=.\IDGenerator.cpp
# End Source File
# Begin Source File

SOURCE=.\IDGenSet.cpp
# End Source File
# Begin Source File

SOURCE=.\kill_list.cpp
# End Source File
# Begin Source File

SOURCE=.\ListDisplay.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadingMap.cpp
# End Source File
# Begin Source File

SOURCE=.\loc_list.cpp
# End Source File
# Begin Source File

SOURCE=.\log_list.cpp
# End Source File
# Begin Source File

SOURCE=..\zone\Map.cpp
# End Source File
# Begin Source File

SOURCE=.\merchant_list.cpp
# End Source File
# Begin Source File

SOURCE=.\mob_list.cpp
# End Source File
# Begin Source File

SOURCE=.\npc_list.cpp
# End Source File
# Begin Source File

SOURCE=.\spawn_list.cpp
# End Source File
# Begin Source File

SOURCE=.\SqlOptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\teleport_list.cpp
# End Source File
# Begin Source File

SOURCE=.\TextLogParser.cpp
# End Source File
# Begin Source File

SOURCE=..\common\TextMapFile.cpp
# End Source File
# Begin Source File

SOURCE=.\waypoint_list.cpp
# End Source File
# Begin Source File

SOURCE=.\zone_list.cpp
# End Source File
# Begin Source File

SOURCE=.\ZoneViewer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BigZoneDlg.h
# End Source File
# Begin Source File

SOURCE=.\BrowseForFolder.h
# End Source File
# Begin Source File

SOURCE=..\common\buildfile.h
# End Source File
# Begin Source File

SOURCE=.\cdoor.h
# End Source File
# Begin Source File

SOURCE=.\cgrid.h
# End Source File
# Begin Source File

SOURCE=.\ckill.h
# End Source File
# Begin Source File

SOURCE=.\cloc.h
# End Source File
# Begin Source File

SOURCE=.\clog.h
# End Source File
# Begin Source File

SOURCE=.\cmerchant.h
# End Source File
# Begin Source File

SOURCE=.\cmob.h
# End Source File
# Begin Source File

SOURCE=.\cnpc.h
# End Source File
# Begin Source File

SOURCE=.\CPathSplit.h
# End Source File
# Begin Source File

SOURCE=.\cspawn.h
# End Source File
# Begin Source File

SOURCE=.\cteleport.h
# End Source File
# Begin Source File

SOURCE=.\cwaypoint.h
# End Source File
# Begin Source File

SOURCE=.\czone.h
# End Source File
# Begin Source File

SOURCE=.\database.h
# End Source File
# Begin Source File

SOURCE=.\DBConnect.h
# End Source File
# Begin Source File

SOURCE=.\door_list.h
# End Source File
# Begin Source File

SOURCE=.\EQBuilder.h
# End Source File
# Begin Source File

SOURCE=.\EQBuilderDlg.h
# End Source File
# Begin Source File

SOURCE=.\eqbuilderini.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\grid_list.h
# End Source File
# Begin Source File

SOURCE=.\IDGenerator.h
# End Source File
# Begin Source File

SOURCE=.\IDGenSet.h
# End Source File
# Begin Source File

SOURCE=.\kill_list.h
# End Source File
# Begin Source File

SOURCE=.\linked_list.h
# End Source File
# Begin Source File

SOURCE=.\LoadingMap.h
# End Source File
# Begin Source File

SOURCE=.\loc_list.h
# End Source File
# Begin Source File

SOURCE=.\log_list.h
# End Source File
# Begin Source File

SOURCE=.\merchant_list.h
# End Source File
# Begin Source File

SOURCE=.\mob_list.h
# End Source File
# Begin Source File

SOURCE=.\npc_list.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\spawn_list.h
# End Source File
# Begin Source File

SOURCE=.\SqlOptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\teleport_list.h
# End Source File
# Begin Source File

SOURCE=..\common\TextMapFile.h
# End Source File
# Begin Source File

SOURCE=.\types.h
# End Source File
# Begin Source File

SOURCE=.\waypoint_list.h
# End Source File
# Begin Source File

SOURCE=.\zone_list.h
# End Source File
# Begin Source File

SOURCE=.\ZoneViewer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\EQBuilder.ico
# End Source File
# Begin Source File

SOURCE=.\res\EQBuilder.rc2
# End Source File
# Begin Source File

SOURCE=.\eqtools.ico
# End Source File
# End Group
# Begin Group "Regex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\regex32\Regexp.cpp
# End Source File
# Begin Source File

SOURCE=.\regex32\Regexp.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\o.txt
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
