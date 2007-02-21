; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CEQBuilderDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "EQBuilder.h"

ClassCount=11
Class1=CEQBuilderApp
Class2=CEQBuilderDlg

ResourceCount=10
Resource1=IDR_MAINFRAME
Class3=log
Resource2=IDD_DATABASE
Resource3=IDD_LOOT
Class4=DBConnect
Resource4=IDD_FACTIONS
Class5=SqlOptionsDlg
Resource5=IDR_MENU1
Resource6=IDD_EQBUILDER_DIALOG
Resource7=IDD_BIGZONE
Resource8=IDD_SQLOPT
Class6=CZoneViewer
Class7=Joe
Resource9=IDD_LOADING_MAP
Class8=BigZoneDlg
Class9=FactionsDialog
Class10=LootDialog
Class11=LoadingMap
Resource10=IDR_MENU1 (French (France))

[CLS:CEQBuilderApp]
Type=0
HeaderFile=EQBuilder.h
ImplementationFile=EQBuilder.cpp
Filter=N

[CLS:CEQBuilderDlg]
Type=0
HeaderFile=EQBuilderDlg.h
ImplementationFile=EQBuilderDlg.cpp
Filter=D
LastObject=IDC_PROGRESS_TEXT
BaseClass=CDialog
VirtualFilter=dWC



[DLG:IDD_EQBUILDER_DIALOG]
Type=1
Class=CEQBuilderDlg
ControlCount=83
Control1=IDC_ZONE_COMBO,combobox,1344341315
Control2=IDC_LOADDB_BUTTON,button,1476460544
Control3=IDC_RELOAD_MAP,button,1476460544
Control4=IDC_GO_BUTTON,button,1342242816
Control5=IDC_LOG_TREE,SysTreeView32,1350631456
Control6=IDC_ADD_BUTTON,button,1476460544
Control7=IDC_ADD_DIR_BUTTON,button,1476460544
Control8=IDC_TYPELOG_COMBO,combobox,1478557699
Control9=IDC_DELETE_BUTTON,button,1476460544
Control10=IDC_DELETE_ALL_BUTTON,button,1476460544
Control11=IDC_COMPILE_BUTTON,button,1476460544
Control12=IDC_COMPILE_ALL_BUTTON,button,1476460544
Control13=IDC_MOVING_CHECK,button,1342242819
Control14=IDC_AROUND_CHECK,button,1342242819
Control15=IDC_CAMP_CHECK,button,1342242819
Control16=IDC_FIXED_MERGE,button,1342242819
Control17=IDC_PATHING_MERGE,button,1342242819
Control18=IDC_DELTAZ_CHECK,button,1342242819
Control19=IDC_MINPROB_CHECK,button,1342242819
Control20=IDC_AFFIRM_CHECK,button,1342242819
Control21=IDC_MOVED_CHECK,button,1342242819
Control22=IDC_DEAD_CHECK,button,1342242819
Control23=IDC_OCCUR_CHECK,button,1342242819
Control24=IDC_AROUND_EDIT,edit,1350631552
Control25=IDC_CAMP_EDIT,edit,1350631552
Control26=IDC_FIXED_MERGE_EDIT,edit,1350631552
Control27=IDC_PATH_MERGE_EDIT,edit,1350631552
Control28=IDC_DELTAZ_EDIT,edit,1350631552
Control29=IDC_MINPROB_EDIT,edit,1350631552
Control30=IDC_AFFIRM_EDIT,edit,1350631552
Control31=IDC_MOVED_EDIT,edit,1350631552
Control32=IDC_DEAD_EDIT,edit,1350631552
Control33=IDC_OCCUR_EDIT,edit,1350631552
Control34=IDC_MOB_TREE,SysTreeView32,1350631463
Control35=IDC_DATAMODE,SysTabControl32,1342377984
Control36=IDC_PROCESS_BUTTON,button,1476460544
Control37=IDC_SQL_RADIO,button,1342308361
Control38=IDC_DATABASE_RADIO,button,1476526089
Control39=IDC_OUT_NPCS,button,1342242819
Control40=IDC_OUT_SPAWNS,button,1342242819
Control41=IDC_OUT_GRIDS,button,1342242819
Control42=IDC_OUT_MERCHANTS,button,1342242819
Control43=IDC_OUT_DOORS,button,1342242819
Control44=IDC_OUT_TELEPORTS,button,1342242819
Control45=IDC_WRITE_BUTTON,button,1476460544
Control46=IDC_ZONE_ZOOM_IN,button,1342242816
Control47=IDC_ZONE_ZOOM_OUT,button,1342242816
Control48=IDC_ZONE_RESET,button,1342242816
Control49=IDC_ZONE_BIG,button,1342242816
Control50=IDC_SPAWN_TREE,SysTreeView32,1350631463
Control51=IDC_PROGRESS,msctls_progress32,1350565889
Control52=IDC_LOG_STATIC,button,1342309127
Control53=IDC_ZONE_STATIC,button,1342309127
Control54=IDC_STATS_STATIC,button,1342178055
Control55=IDC_STATIC,static,1342308353
Control56=IDC_NPC_STATIC,static,1342308353
Control57=IDC_STATIC,static,1342308353
Control58=IDC_MOB_STATIC,static,1342308353
Control59=IDC_STATIC,static,1342308353
Control60=IDC_SPAWN_STATIC,static,1342308353
Control61=ID_STATIC,static,1342308353
Control62=IDC_DOOR_STATIC,static,1342308353
Control63=IDC_STATIC,static,1342308353
Control64=IDC_TELEPORT_STATIC,static,1342308353
Control65=IDC_STATIC,button,1342178055
Control66=IDC_STATIC,button,1342309127
Control67=IDC_STATIC,button,1342178055
Control68=IDC_STATIC,static,1342308353
Control69=IDC_GRID_STATIC,static,1342308353
Control70=IDC_STATIC,static,1342308353
Control71=IDC_MERCHANT_STATIC,static,1342308353
Control72=IDC_STATIC,static,1342308352
Control73=IDC_ZONEVIEW,MFCZoneViewerCtrl,1342177280
Control74=IDC_STATIC,button,1342177287
Control75=IDC_FACTIONS_BUTTON,button,1342242816
Control76=IDC_LOOT_BUTTON,button,1342242816
Control77=IDC_SPELLS_BUTTON,button,1342242816
Control78=IDC_RESET_IDS,button,1342242816
Control79=IDC_SAVE_IDS,button,1342242816
Control80=IDC_LOAD_IDS,button,1342242816
Control81=IDC_PROGRESS_TEXT,edit,1484849280
Control82=IDC_STATIC,static,1342308353
Control83=IDC_RACE_ERRORS,static,1342308353

[CLS:log]
Type=0
HeaderFile=log.h
ImplementationFile=log.cpp
BaseClass=generic CWnd
Filter=W

[MNU:IDR_MENU1]
Type=1
Class=CEQBuilderDlg
Command1=ID_DATABASE_CONNECTION
Command2=ID_OUTPUT_SQLOPT
Command3=ID_OUTPUT_DBOPT
CommandCount=3

[DLG:IDD_DATABASE]
Type=1
Class=DBConnect
ControlCount=10
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_HOST,edit,1350631552
Control4=IDC_USER,edit,1350631552
Control5=IDC_PASSWORD,edit,1350631552
Control6=IDC_DATABASE,edit,1350631552
Control7=IDC_STATIC,static,1342308353
Control8=IDC_STATIC,static,1342308353
Control9=IDC_STATIC,static,1342308353
Control10=IDC_STATIC,static,1342308353

[CLS:DBConnect]
Type=0
HeaderFile=DBConnect.h
ImplementationFile=DBConnect.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=DBConnect

[DLG:IDD_SQLOPT]
Type=1
Class=SqlOptionsDlg
ControlCount=58
Control1=IDC_DELETE_CHECK,button,1342242819
Control2=IDC_STATIC,static,1342308352
Control3=IDC_NPCID_DB,button,1342177289
Control4=IDC_NPCID_FIXED,button,1342177321
Control5=IDC_NPCID_FLOAT,button,1342177321
Control6=IDC_NPCID_ZID,button,1342177321
Control7=IDC_STATIC,static,1342308352
Control8=IDC_SPAWN2ID_DB,button,1342177289
Control9=IDC_SPAWN2ID_FIXED,button,1342177321
Control10=IDC_SPAWN2ID_FLOAT,button,1342177321
Control11=IDC_SPAWN2ID_ZID,button,1342177321
Control12=IDC_STATIC,static,1342308352
Control13=IDC_GRIDID_DB,button,1342177289
Control14=IDC_GRIDID_FIXED,button,1342177321
Control15=IDC_GRIDID_FLOAT,button,1342177321
Control16=IDC_GRIDID_ZID,button,1342177321
Control17=IDC_STATIC,static,1342308352
Control18=IDC_GRIDID_DB2,button,1342177289
Control19=IDC_GRIDID_FIXED2,button,1342177321
Control20=IDC_SPAWNID_FLOAT3,button,1342177321
Control21=IDC_MERCHANTID_ZID,button,1342177321
Control22=IDC_NPCID,edit,1484849280
Control23=IDC_NPCID_FVAL,edit,1484849280
Control24=IDC_NPCID_ZVAL,edit,1484849280
Control25=IDC_SPAWN2ID,edit,1484849280
Control26=IDC_SPAWN2ID_FVAL,edit,1484849280
Control27=IDC_SPAWN2ID_ZVAL,edit,1484849280
Control28=IDC_GRIDID,edit,1484849280
Control29=IDC_GRIDID_FVAL,edit,1484849280
Control30=IDC_GRIDID_ZVAL,edit,1484849280
Control31=IDC_MERCHANTID,edit,1484849280
Control32=IDC_MERCHANTID_FVAL,edit,1484849280
Control33=IDC_MERCHANTID_ZVAL,edit,1484849280
Control34=IDC_EQMAPS,edit,1350631552
Control35=IDC_BrowseMaps,button,1342242816
Control36=IDC_EQEMUMAPS,edit,1350631552
Control37=IDC_BrowseEmuMaps,button,1342242816
Control38=IDOK,button,1342242817
Control39=ID_STATIC,button,1342177287
Control40=ID_STATIC2,button,1342177287
Control41=IDC_STATIC,static,1342308352
Control42=IDC_STATIC,static,1342308352
Control43=IDC_STATIC,button,1342177287
Control44=IDC_STATIC,static,1342308352
Control45=IDC_STATIC,static,1342308352
Control46=IDC_STATIC,static,1342308352
Control47=IDC_STATIC,static,1342308352
Control48=IDC_FIXED_FROM_DB,button,1342242816
Control49=IDC_FLOAT_FROM_DB,button,1342242816
Control50=IDC_STATIC,static,1342308352
Control51=IDC_STATIC,static,1342308352
Control52=IDC_SPAWNGROUPID_DB,button,1342177289
Control53=IDC_SPAWNGROUPID_FIXED,button,1342177321
Control54=IDC_SPAWNGROUPID_FLOAT,button,1342177321
Control55=IDC_SPAWNGROUPID_ZID,button,1342177321
Control56=IDC_SPAWNGROUPID,edit,1484849280
Control57=IDC_SPAWNGROUPID_FVAL,edit,1484849280
Control58=IDC_SPAWNGROUPID_ZVAL,edit,1484849280

[CLS:SqlOptionsDlg]
Type=0
HeaderFile=SqlOptionsDlg.h
ImplementationFile=SqlOptionsDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=ID_DATABASE_CONNECTION
VirtualFilter=dWC

[MNU:IDR_MENU1 (French (France))]
Type=1
Class=?
Command1=ID_DATABASE_CONNECTION
Command2=ID_OUTPUT_SQLOPT
Command3=ID_OUTPUT_DBOPT
CommandCount=3

[CLS:CZoneViewer]
Type=0
HeaderFile=zoneviewer.h
ImplementationFile=zoneviewer.cpp
BaseClass=CWnd
LastObject=CZoneViewer

[CLS:Joe]
Type=0
HeaderFile=Joe.h
ImplementationFile=Joe.cpp
BaseClass=generic CWnd
Filter=W
LastObject=ID_DATABASE_CONNECTION

[DLG:IDD_BIGZONE]
Type=1
Class=BigZoneDlg
ControlCount=15
Control1=IDOK,button,1342242817
Control2=IDC_ZONEVIEW,MFCZoneViewerCtrl,1342242816
Control3=IDC_ZoomIn,button,1342242817
Control4=IDC_ZoomOut,button,1342242817
Control5=IDC_RESET_VIEW,button,1342242817
Control6=IDC_STATIC,button,1342177287
Control7=IDC_STATIC,button,1342177287
Control8=IDC_REPROCESS,button,1342242816
Control9=IDC_INCREASE_RES,button,1342242817
Control10=IDC_DECREASE_RES,button,1342242817
Control11=IDC_DRAW_PATHS,button,1342242819
Control12=IDC_MOB_TREE,SysTreeView32,1350635527
Control13=IDC_STATIC,static,1342308352
Control14=IDC_DRAW_HILITE,button,1342242819
Control15=IDC_STATIC,static,1342308352

[CLS:BigZoneDlg]
Type=0
HeaderFile=BigZoneDlg.h
ImplementationFile=BigZoneDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=BigZoneDlg

[DLG:IDD_LOOT]
Type=1
Class=LootDialog
ControlCount=10
Control1=IDOK,button,1342242817
Control2=IDC_SOURCE_MAPPINGS,SysTreeView32,1350631424
Control3=IDC_STATIC,static,1342308352
Control4=IDC_CURRENT_MAPPINGS,SysTreeView32,1350631424
Control5=IDC_STATIC,static,1342308352
Control6=IDC_DELETE_MAPPING,button,1342242816
Control7=IDC_ADD_MAPPING,button,1342242816
Control8=IDC_TOGGLE_LOOT,button,1342242819
Control9=IDC_EDIT_MAPPING,button,1342242816
Control10=IDC_STATIC,button,1342177287

[DLG:IDD_FACTIONS]
Type=1
Class=FactionsDialog
ControlCount=9
Control1=IDOK,button,1342242817
Control2=IDC_SOURCE_MAPPINGS,SysTreeView32,1350631424
Control3=IDC_STATIC,static,1342308352
Control4=IDC_CURRENT_MAPPINGS,SysTreeView32,1350631424
Control5=IDC_STATIC,static,1342308352
Control6=IDC_TOGGLE_FACTIONS,button,1342242819
Control7=IDC_DELETE_MAPPING,button,1342242816
Control8=IDC_ADD_MAPPING,button,1342242816
Control9=IDC_EDIT_MAPPINGS,button,1342242816

[CLS:FactionsDialog]
Type=0
HeaderFile=FactionsDialog.h
ImplementationFile=FactionsDialog.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_SOURCE_MAPPINGS
VirtualFilter=dWC

[CLS:LootDialog]
Type=0
HeaderFile=LootDialog.h
ImplementationFile=LootDialog.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_SOURCE_MAPPINGS
VirtualFilter=dWC



[DLG:IDD_LOADING_MAP]
Type=1
Class=LoadingMap
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[CLS:LoadingMap]
Type=0
HeaderFile=LoadingMap.h
ImplementationFile=LoadingMap.cpp
BaseClass=CDialog
Filter=D
LastObject=ID_DATABASE_CONNECTION

