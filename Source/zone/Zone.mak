# Microsoft Developer Studio Generated NMAKE File, Based on Zone.dsp
!IF "$(CFG)" == ""
CFG=Zone - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Zone - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Zone - Win32 Release" && "$(CFG)" != "Zone - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Zone.mak" CFG="Zone - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Zone - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Zone - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Zone - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Zone.exe" "$(OUTDIR)\Zone.bsc"


CLEAN :
	-@erase "$(INTDIR)\attack.obj"
	-@erase "$(INTDIR)\attack.sbr"
	-@erase "$(INTDIR)\classes.obj"
	-@erase "$(INTDIR)\classes.sbr"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\client.sbr"
	-@erase "$(INTDIR)\client_process.obj"
	-@erase "$(INTDIR)\client_process.sbr"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\database.sbr"
	-@erase "$(INTDIR)\entity.obj"
	-@erase "$(INTDIR)\entity.sbr"
	-@erase "$(INTDIR)\EQFragment.obj"
	-@erase "$(INTDIR)\EQFragment.sbr"
	-@erase "$(INTDIR)\EQPacket.obj"
	-@erase "$(INTDIR)\EQPacket.sbr"
	-@erase "$(INTDIR)\EQPacketManager.obj"
	-@erase "$(INTDIR)\EQPacketManager.sbr"
	-@erase "$(INTDIR)\hate_list.obj"
	-@erase "$(INTDIR)\hate_list.sbr"
	-@erase "$(INTDIR)\mob.obj"
	-@erase "$(INTDIR)\mob.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\npc.obj"
	-@erase "$(INTDIR)\npc.sbr"
	-@erase "$(INTDIR)\packet_dump.obj"
	-@erase "$(INTDIR)\packet_dump.sbr"
	-@erase "$(INTDIR)\packet_functions.obj"
	-@erase "$(INTDIR)\packet_functions.sbr"
	-@erase "$(INTDIR)\races.obj"
	-@erase "$(INTDIR)\races.sbr"
	-@erase "$(INTDIR)\spawn.obj"
	-@erase "$(INTDIR)\spawn.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\worldserver.obj"
	-@erase "$(INTDIR)\worldserver.sbr"
	-@erase "$(INTDIR)\zone.obj"
	-@erase "$(INTDIR)\zone.sbr"
	-@erase "$(OUTDIR)\Zone.bsc"
	-@erase "$(OUTDIR)\Zone.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /w /W0 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Zone.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Zone.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\attack.sbr" \
	"$(INTDIR)\client.sbr" \
	"$(INTDIR)\client_process.sbr" \
	"$(INTDIR)\entity.sbr" \
	"$(INTDIR)\hate_list.sbr" \
	"$(INTDIR)\mob.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\npc.sbr" \
	"$(INTDIR)\spawn.sbr" \
	"$(INTDIR)\worldserver.sbr" \
	"$(INTDIR)\zone.sbr" \
	"$(INTDIR)\classes.sbr" \
	"$(INTDIR)\database.sbr" \
	"$(INTDIR)\EQFragment.sbr" \
	"$(INTDIR)\EQPacket.sbr" \
	"$(INTDIR)\EQPacketManager.sbr" \
	"$(INTDIR)\packet_dump.sbr" \
	"$(INTDIR)\packet_functions.sbr" \
	"$(INTDIR)\races.sbr" \
	"$(INTDIR)\timer.sbr"

"$(OUTDIR)\Zone.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\Zone.pdb" /machine:I386 /out:"$(OUTDIR)\Zone.exe" 
LINK32_OBJS= \
	"$(INTDIR)\attack.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\client_process.obj" \
	"$(INTDIR)\entity.obj" \
	"$(INTDIR)\hate_list.obj" \
	"$(INTDIR)\mob.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\npc.obj" \
	"$(INTDIR)\spawn.obj" \
	"$(INTDIR)\worldserver.obj" \
	"$(INTDIR)\zone.obj" \
	"$(INTDIR)\classes.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\EQFragment.obj" \
	"$(INTDIR)\EQPacket.obj" \
	"$(INTDIR)\EQPacketManager.obj" \
	"$(INTDIR)\packet_dump.obj" \
	"$(INTDIR)\packet_functions.obj" \
	"$(INTDIR)\races.obj" \
	"$(INTDIR)\timer.obj"

"$(OUTDIR)\Zone.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Zone.exe"


CLEAN :
	-@erase "$(INTDIR)\attack.obj"
	-@erase "$(INTDIR)\classes.obj"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\client_process.obj"
	-@erase "$(INTDIR)\database.obj"
	-@erase "$(INTDIR)\entity.obj"
	-@erase "$(INTDIR)\EQFragment.obj"
	-@erase "$(INTDIR)\EQPacket.obj"
	-@erase "$(INTDIR)\EQPacketManager.obj"
	-@erase "$(INTDIR)\hate_list.obj"
	-@erase "$(INTDIR)\mob.obj"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\npc.obj"
	-@erase "$(INTDIR)\packet_dump.obj"
	-@erase "$(INTDIR)\packet_functions.obj"
	-@erase "$(INTDIR)\races.obj"
	-@erase "$(INTDIR)\spawn.obj"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\worldserver.obj"
	-@erase "$(INTDIR)\zone.obj"
	-@erase "$(OUTDIR)\Zone.exe"
	-@erase "$(OUTDIR)\Zone.ilk"
	-@erase "$(OUTDIR)\Zone.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_EQDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /Fp"$(INTDIR)\Zone.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Zone.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\Zone.pdb" /debug /machine:I386 /out:"$(OUTDIR)\Zone.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\attack.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\client_process.obj" \
	"$(INTDIR)\entity.obj" \
	"$(INTDIR)\hate_list.obj" \
	"$(INTDIR)\mob.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\npc.obj" \
	"$(INTDIR)\spawn.obj" \
	"$(INTDIR)\worldserver.obj" \
	"$(INTDIR)\zone.obj" \
	"$(INTDIR)\classes.obj" \
	"$(INTDIR)\database.obj" \
	"$(INTDIR)\EQFragment.obj" \
	"$(INTDIR)\EQPacket.obj" \
	"$(INTDIR)\EQPacketManager.obj" \
	"$(INTDIR)\packet_dump.obj" \
	"$(INTDIR)\packet_functions.obj" \
	"$(INTDIR)\races.obj" \
	"$(INTDIR)\timer.obj"

"$(OUTDIR)\Zone.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Zone.dep")
!INCLUDE "Zone.dep"
!ELSE 
!MESSAGE Warning: cannot find "Zone.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Zone - Win32 Release" || "$(CFG)" == "Zone - Win32 Debug"
SOURCE=.\attack.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\attack.obj"	"$(INTDIR)\attack.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\attack.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\client.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\client.obj"	"$(INTDIR)\client.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\client.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\client_process.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\client_process.obj"	"$(INTDIR)\client_process.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\client_process.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\entity.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\entity.obj"	"$(INTDIR)\entity.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\entity.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\hate_list.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\hate_list.obj"	"$(INTDIR)\hate_list.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\hate_list.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mob.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\mob.obj"	"$(INTDIR)\mob.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\mob.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\net.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\net.obj"	"$(INTDIR)\net.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\net.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\npc.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\npc.obj"	"$(INTDIR)\npc.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\npc.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\spawn.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\spawn.obj"	"$(INTDIR)\spawn.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\spawn.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\worldserver.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\worldserver.obj"	"$(INTDIR)\worldserver.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\worldserver.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\zone.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\zone.obj"	"$(INTDIR)\zone.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\zone.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\common\classes.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\classes.obj"	"$(INTDIR)\classes.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\classes.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\database.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"

CPP_SWITCHES=/nologo /MT /w /W0 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Zone.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\database.obj"	"$(INTDIR)\database.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"

CPP_SWITCHES=/nologo /Gd /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_EQDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /Fp"$(INTDIR)\Zone.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\database.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\common\EQFragment.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\EQFragment.obj"	"$(INTDIR)\EQFragment.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\EQFragment.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\EQPacket.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\EQPacket.obj"	"$(INTDIR)\EQPacket.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\EQPacket.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\EQPacketManager.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\EQPacketManager.obj"	"$(INTDIR)\EQPacketManager.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\EQPacketManager.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\packet_dump.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\packet_dump.obj"	"$(INTDIR)\packet_dump.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\packet_dump.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\packet_functions.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\packet_functions.obj"	"$(INTDIR)\packet_functions.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\packet_functions.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\races.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\races.obj"	"$(INTDIR)\races.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\races.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\timer.cpp

!IF  "$(CFG)" == "Zone - Win32 Release"


"$(INTDIR)\timer.obj"	"$(INTDIR)\timer.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Zone - Win32 Debug"


"$(INTDIR)\timer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

