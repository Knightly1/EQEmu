# Microsoft Developer Studio Project File - Name="Zone" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ZONE - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Zone.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Zone.mak" CFG="ZONE - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Zone - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 GotFrags" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 Win9x" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 Profiling" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 GuildWars" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 GuildWars Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Zone - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build/"
# PROP Intermediate_Dir "../Build/Zone/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /Zi /O2 /Ob2 /D "SHAREMEM" /D "CATCH_CRASH" /D _WIN32_WINNT=0x0400 /D "NDEBUG" /D "ZONESERVER" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "FIELD_ITEMS" /D EQDEBUG=0 /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Build/Zone/Zone.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/Zone.map" /debug /machine:I386 /nodefaultlib:"libc"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../build/"
# PROP Intermediate_Dir "../Build/Zone/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /Gm /GX /ZI /Od /D "SHAREMEM" /D _WIN32_WINNT=0x0400 /D "_EQDEBUG" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EQDEBUG" /D "FIELD_ITEMS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"LIBCMT" /nodefaultlib:"LIBC" /out:"../build/ZoneDebug.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Zone - Win32 GotFrags"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Zone___Win32_GotFrags"
# PROP BASE Intermediate_Dir "Zone___Win32_GotFrags"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build/"
# PROP Intermediate_Dir "../Build/Zone/GotFrags"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /D "LUCLIN" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /Od /Ob2 /D "GOTFRAGS" /D "SHAREMEM" /D _WIN32_WINNT=0x0400 /D "NDEBUG" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EQDEBUG 0" /D "FIELD_ITEMS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Zone/Zone.bsc"
# ADD BSC32 /nologo /o"../Build/Zone/Zone.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386 /out:"../Build/ZoneNT.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/ZoneGF.map" /debug /machine:I386 /out:"..\build\\ZoneGF.exe"

!ELSEIF  "$(CFG)" == "Zone - Win32 Win9x"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Zone___Win32_Win9x"
# PROP BASE Intermediate_Dir "Zone___Win32_Win9x"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "../Build/Zone/Win9x"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /Od /Ob2 /D "INVERSEXY" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D "_WIN9x_" /D "NDEBUG" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EQDEBUG 0" /D "FIELD_ITEMS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Zone/Zone.bsc"
# ADD BSC32 /nologo /o"../Build/Zone9x/Zone9x.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /debug /machine:I386 /out:"C:\eqemucvs\build\\ZoneGF.exe"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386 /out:"C:\eqemucvs\Source\build\\Zone9x.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Zone - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Zone___Win32_Profiling"
# PROP BASE Intermediate_Dir "Zone___Win32_Profiling"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../build/"
# PROP Intermediate_Dir "../build/Zone/Profiling"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D "SHAREMEM" /D "CATCH_CRASH" /D "INVERSEXY" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "ZONESERVER" /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D "SHAREMEM" /D "CATCH_CRASH" /D _WIN32_WINNT=0x0400 /D "NDEBUG" /D "ZONESERVER" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D EQDEBUG=0 /D "FIELD_ITEMS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Zone/Zone.bsc"
# ADD BSC32 /nologo /o"../Build/Zone/Zone.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386 /out:"../Build/ZoneNT.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /profile /machine:I386 /nodefaultlib:"libc" /out:"C:\EQEmuCVS\Source\Build\Zone.exe"

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Zone___Win32_GuildWars"
# PROP BASE Intermediate_Dir "Zone___Win32_GuildWars"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build/"
# PROP Intermediate_Dir "../Build/Zone/GuildWars"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /Zi /O2 /Ob2 /D "ZONE" /D "SHAREMEM" /D "CATCH_CRASH" /D "INVERSEXY" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "ZONESERVER" /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /Zi /O2 /Ob2 /D "SHAREMEM" /D "CATCH_CRASH" /D _WIN32_WINNT=0x0400 /D "NDEBUG" /D "ZONESERVER" /D "GUILDWARS" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EQDEBUG 0" /D "FIELD_ITEMS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Zone/Zone.bsc"
# ADD BSC32 /nologo /o"../Build/Zone/Zone.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/Zone.map" /debug /machine:I386 /nodefaultlib:"libc"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/Zone.map" /debug /machine:I386 /nodefaultlib:"libc"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Zone___Win32_GuildWars_Debug"
# PROP BASE Intermediate_Dir "Zone___Win32_GuildWars_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Zone___Win32_GuildWars_Debug"
# PROP Intermediate_Dir "Zone___Win32_GuildWars_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /Zi /O2 /Ob2 /D "ZONE" /D "SHAREMEM" /D "CATCH_CRASH" /D "INVERSEXY" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "ZONESERVER" /D "GUILDWARS" /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /Zi /O2 /Ob2 /D "SHAREMEM" /D "CATCH_CRASH" /D _WIN32_WINNT=0x0400 /D "NDEBUG" /D "ZONESERVER" /D "ZONE" /D "INVERSEXY" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EQDEBUG 0" /D "FIELD_ITEMS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Zone/Zone.bsc"
# ADD BSC32 /nologo /o"../Build/Zone/Zone.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/Zone.map" /debug /machine:I386 /nodefaultlib:"libc"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/Zone.map" /debug /machine:I386 /nodefaultlib:"libc"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Zone - Win32 Release"
# Name "Zone - Win32 Debug"
# Name "Zone - Win32 GotFrags"
# Name "Zone - Win32 Win9x"
# Name "Zone - Win32 Profiling"
# Name "Zone - Win32 GuildWars"
# Name "Zone - Win32 GuildWars Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\attack.cpp
# End Source File
# Begin Source File

SOURCE=.\beacon.cpp
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\client_process.cpp
# End Source File
# Begin Source File

SOURCE=.\command.cpp
# End Source File
# Begin Source File

SOURCE=.\doors.cpp
# End Source File
# Begin Source File

SOURCE=.\entity.cpp
# End Source File
# Begin Source File

SOURCE=..\common\EQEMuError.cpp
# End Source File
# Begin Source File

SOURCE=.\faction.cpp
# End Source File
# Begin Source File

SOURCE=.\forage.cpp
# End Source File
# Begin Source File

SOURCE=.\groups.cpp
# End Source File
# Begin Source File

SOURCE=.\hate_list.cpp
# End Source File
# Begin Source File

SOURCE=.\loottables.cpp
# End Source File
# Begin Source File

SOURCE=.\Map.cpp
# End Source File
# Begin Source File

SOURCE=.\mob.cpp
# End Source File
# Begin Source File

SOURCE=.\MobAI.cpp
# End Source File
# Begin Source File

SOURCE=.\net.cpp
# End Source File
# Begin Source File

SOURCE=.\npc.cpp
# End Source File
# Begin Source File

SOURCE=.\Object.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\petitions.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerCorpse.cpp
# End Source File
# Begin Source File

SOURCE=.\spawn2.cpp
# End Source File
# Begin Source File

SOURCE=.\spawngroup.cpp
# End Source File
# Begin Source File

SOURCE=.\spells.cpp
# End Source File
# Begin Source File

SOURCE=.\worldserver.cpp
# End Source File
# Begin Source File

SOURCE=.\zone.cpp
# End Source File
# Begin Source File

SOURCE=.\zonedbasync.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\beacon.h
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\command.h
# End Source File
# Begin Source File

SOURCE=.\doors.h
# End Source File
# Begin Source File

SOURCE=.\entity.h
# End Source File
# Begin Source File

SOURCE=..\common\EQEMuError.h
# End Source File
# Begin Source File

SOURCE=.\event_codes.h
# End Source File
# Begin Source File

SOURCE=.\faction.h
# End Source File
# Begin Source File

SOURCE=..\common\files.h
# End Source File
# Begin Source File

SOURCE=.\Forage.h
# End Source File
# Begin Source File

SOURCE=.\groups.h
# End Source File
# Begin Source File

SOURCE=.\hate_list.h
# End Source File
# Begin Source File

SOURCE=.\loottable.h
# End Source File
# Begin Source File

SOURCE=.\map.h
# End Source File
# Begin Source File

SOURCE=.\mob.h
# End Source File
# Begin Source File

SOURCE=.\net.h
# End Source File
# Begin Source File

SOURCE=.\npc.h
# End Source File
# Begin Source File

SOURCE=.\NpcAI.h
# End Source File
# Begin Source File

SOURCE=.\object.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\petitions.h
# End Source File
# Begin Source File

SOURCE=.\PlayerCorpse.h
# End Source File
# Begin Source File

SOURCE=.\skills.h
# End Source File
# Begin Source File

SOURCE=.\spawn2.h
# End Source File
# Begin Source File

SOURCE=.\spawngroup.h
# End Source File
# Begin Source File

SOURCE=.\spdat.h
# End Source File
# Begin Source File

SOURCE=.\worldserver.h
# End Source File
# Begin Source File

SOURCE=.\zone.h
# End Source File
# Begin Source File

SOURCE=.\zonedbasync.h
# End Source File
# Begin Source File

SOURCE=.\zonedump.h
# End Source File
# End Group
# Begin Group "Common Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\classes.h
# End Source File
# Begin Source File

SOURCE=..\common\crc32.h
# End Source File
# Begin Source File

SOURCE=..\common\database.h
# End Source File
# Begin Source File

SOURCE=..\common\dbasync.h
# End Source File
# Begin Source File

SOURCE=..\common\dbcore.h
# End Source File
# Begin Source File

SOURCE=..\common\DBMemLeak.h
# End Source File
# Begin Source File

SOURCE=..\common\debug.h
# End Source File
# Begin Source File

SOURCE=..\common\deity.h
# End Source File
# Begin Source File

SOURCE=..\common\EMuShareMem.h
# End Source File
# Begin Source File

SOURCE=..\common\eq_opcodes.h
# End Source File
# Begin Source File

SOURCE=..\common\eq_packet_structs.h
# End Source File
# Begin Source File

SOURCE=..\common\EQCheckTable.h
# End Source File
# Begin Source File

SOURCE=..\common\EQNetwork.h
# End Source File
# Begin Source File

SOURCE=..\common\EQOpcodes.h
# End Source File
# Begin Source File

SOURCE=..\common\eqtime.h
# End Source File
# Begin Source File

SOURCE=..\common\Guilds.h
# End Source File
# Begin Source File

SOURCE=..\common\Item.h
# End Source File
# Begin Source File

SOURCE="..\common\Kaiyodo-LList.h"
# End Source File
# Begin Source File

SOURCE=..\common\linked_list.h
# End Source File
# Begin Source File

SOURCE=..\common\md5.h
# End Source File
# Begin Source File

SOURCE=..\common\misc.h
# End Source File
# Begin Source File

SOURCE=..\common\MiscFunctions.h
# End Source File
# Begin Source File

SOURCE=..\common\moremath.h
# End Source File
# Begin Source File

SOURCE=..\common\Mutex.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump_file.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_functions.h
# End Source File
# Begin Source File

SOURCE=..\common\queue.h
# End Source File
# Begin Source File

SOURCE=..\common\races.h
# End Source File
# Begin Source File

SOURCE=..\common\Seperator.h
# End Source File
# Begin Source File

SOURCE=..\common\serverinfo.h
# End Source File
# Begin Source File

SOURCE=..\common\servertalk.h
# End Source File
# Begin Source File

SOURCE=..\common\TCPConnection.h
# End Source File
# Begin Source File

SOURCE=..\common\timer.h
# End Source File
# Begin Source File

SOURCE=..\common\types.h
# End Source File
# Begin Source File

SOURCE=..\common\unix.h

!IF  "$(CFG)" == "Zone - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GotFrags"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Win9x"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\version.h
# End Source File
# End Group
# Begin Group "Common Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\classes.cpp
# End Source File
# Begin Source File

SOURCE=..\common\crc32.cpp
# End Source File
# Begin Source File

SOURCE=..\common\database.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

# ADD CPP /Gd /Gm /YX

!ELSEIF  "$(CFG)" == "Zone - Win32 GotFrags"

!ELSEIF  "$(CFG)" == "Zone - Win32 Win9x"

!ELSEIF  "$(CFG)" == "Zone - Win32 Profiling"

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars"

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\dbasync.cpp
# End Source File
# Begin Source File

SOURCE=..\common\dbcore.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DBMemLeak.cpp
# End Source File
# Begin Source File

SOURCE=..\common\debug.cpp
# End Source File
# Begin Source File

SOURCE=..\common\EMuShareMem.cpp
# End Source File
# Begin Source File

SOURCE=..\common\EQNetwork.cpp
# End Source File
# Begin Source File

SOURCE=..\common\eqtime.cpp
# End Source File
# Begin Source File

SOURCE=..\common\guilds.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Item.cpp
# End Source File
# Begin Source File

SOURCE=..\common\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\misc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MiscFunctions.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "Zone - Win32 GotFrags"

!ELSEIF  "$(CFG)" == "Zone - Win32 Win9x"

!ELSEIF  "$(CFG)" == "Zone - Win32 Profiling"

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars"

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\moremath.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Mutex.cpp
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

SOURCE=..\common\races.cpp
# End Source File
# Begin Source File

SOURCE=..\common\serverinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\common\TCPConnection.cpp
# End Source File
# Begin Source File

SOURCE=..\common\timer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\unix.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GotFrags"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Win9x"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\readme.txt

!IF  "$(CFG)" == "Zone - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GotFrags"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Win9x"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 Profiling"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Zone - Win32 GuildWars Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Target
# End Project

