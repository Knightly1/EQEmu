# Microsoft Developer Studio Project File - Name="EQExtractor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=EQExtractor - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EQExtractor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EQExtractor.mak" CFG="EQExtractor - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EQExtractor - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "EQExtractor - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EQExtractor - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "C:\eqemu\include" /I "." /I "c:\mysql\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "COLLECTOR" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 zlib.lib libmysql.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\common" /libpath:"c:\eqemu\lib" /libpath:"c:\mysql\lib\opt"

!ELSEIF  "$(CFG)" == "EQExtractor - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /w /W0 /Gm /GX /ZI /Od /I "C:\eqemu\include" /I "." /I "c:\mysql\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "COLLECTOR" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zlib.lib libmysql.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcmt" /pdbtype:sept /libpath:"..\common" /libpath:"c:\eqemu\lib" /libpath:"c:\mysql\lib\opt"

!ENDIF 

# Begin Target

# Name "EQExtractor - Win32 Release"
# Name "EQExtractor - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\eqextractor.cpp
# End Source File
# Begin Source File

SOURCE=.\Explorers.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractCollector.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractDB.cpp
# End Source File
# Begin Source File

SOURCE=.\StructExplorer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BuildWriter.h
# End Source File
# Begin Source File

SOURCE=.\Explorers.h
# End Source File
# Begin Source File

SOURCE=.\ExtractCollector.h
# End Source File
# Begin Source File

SOURCE=.\ExtractDB.h
# End Source File
# Begin Source File

SOURCE=.\Extractors.h
# End Source File
# Begin Source File

SOURCE=.\StructExplorer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Common Source"

# PROP Default_Filter ".cpp"
# Begin Source File

SOURCE=..\common\buildfile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\dbcore.cpp
# End Source File
# Begin Source File

SOURCE=..\common\emu_opcodes.cpp
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

SOURCE=..\common\opcodemgr.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packetfile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\timer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\win_getopt.cpp
# End Source File
# End Group
# Begin Group "Common Headers"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\common\buildfile.h
# End Source File
# Begin Source File

SOURCE=..\common\dbcore.h
# End Source File
# Begin Source File

SOURCE=..\common\emu_opcodes.h
# End Source File
# Begin Source File

SOURCE=..\common\emu_oplist.h
# End Source File
# Begin Source File

SOURCE=..\common\misc.h
# End Source File
# Begin Source File

SOURCE=..\common\MiscFunctions.h
# End Source File
# Begin Source File

SOURCE=..\common\Mutex.h
# End Source File
# Begin Source File

SOURCE=..\common\opcodemgr.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump.h
# End Source File
# Begin Source File

SOURCE=..\common\packetfile.h
# End Source File
# Begin Source File

SOURCE=..\common\win_getopt.h
# End Source File
# End Group
# Begin Group "Patches"

# PROP Default_Filter ".cpp,.h"
# Begin Source File

SOURCE=.\patches\cleardefs.h
# End Source File
# Begin Source File

SOURCE=.\patches\factory.h
# End Source File
# Begin Source File

SOURCE=.\patches\patch_021505.cpp
# End Source File
# Begin Source File

SOURCE=.\patches\patch_021505.h
# End Source File
# Begin Source File

SOURCE=.\patches\patch_051105.cpp
# End Source File
# Begin Source File

SOURCE=.\patches\patch_051105.h
# End Source File
# Begin Source File

SOURCE=.\patches\patch_121504.cpp
# End Source File
# Begin Source File

SOURCE=.\patches\patch_121504.h
# End Source File
# Begin Source File

SOURCE=.\patches\patch_current.cpp
# End Source File
# Begin Source File

SOURCE=.\patches\patch_current.h
# End Source File
# Begin Source File

SOURCE=.\patches\patch_local.cpp
# End Source File
# Begin Source File

SOURCE=.\patches\patch_local.h
# End Source File
# Begin Source File

SOURCE=.\patches\versions.cpp
# End Source File
# Begin Source File

SOURCE=.\patches\versions.h
# End Source File
# End Group
# End Target
# End Project
