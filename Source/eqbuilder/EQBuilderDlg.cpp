// EQBuilderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include <math.h>
#include  <io.h>
#include "globals.h"
#include "../common/types.h"
#include "ZoneViewer.h"
#include "BigZoneDlg.h"
#include "FactionsDialog.h"
#include "LootDialog.h"
#include "CPathSplit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CEQBuilderDlg dialog

CEQBuilderDlg::CEQBuilderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEQBuilderDlg::IDD, pParent),
	  m_ids(&db_real)
{
	//{{AFX_DATA_INIT(CEQBuilderDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	zones = NULL;
}

void CEQBuilderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEQBuilderDlg)
	DDX_Control(pDX, IDC_PROGRESS_TEXT, m_progressText);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_ZONEVIEW, vZoneView);
}

BEGIN_MESSAGE_MAP(CEQBuilderDlg, CDialog)
	//{{AFX_MSG_MAP(CEQBuilderDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_ZONE_COMBO, OnSelchangeZoneCombo)
	ON_CBN_CLOSEUP(IDC_ZONE_COMBO, OnCloseupZoneCombo)
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_ADD_DIR_BUTTON, OnAddDirButton)
	ON_BN_CLICKED(IDC_DELETE_BUTTON, OnDeleteButton)
	ON_BN_CLICKED(IDC_DELETE_ALL_BUTTON, OnDeleteAllButton)
	ON_BN_CLICKED(IDC_COMPILE_BUTTON, OnCompileButton)
	ON_BN_CLICKED(IDC_COMPILE_ALL_BUTTON, OnCompileAllButton)
	ON_NOTIFY(TVN_SELCHANGED, IDC_LOG_TREE, OnSelchangedLogTree)
	ON_BN_CLICKED(IDC_DOOR_BUTTON, OnDoorButton)
	ON_BN_CLICKED(IDC_SQL_RADIO, OnSqlRadio)
	ON_BN_CLICKED(IDC_DATABASE_RADIO, OnDatabaseRadio)
	ON_BN_CLICKED(IDC_TELEPORT_BUTTON, OnTeleportButton)
	ON_BN_CLICKED(IDC_NPC_BUTTON, OnNpcButton)
	ON_BN_CLICKED(IDC_AROUND_CHECK, OnAroundCheck)
	ON_BN_CLICKED(IDC_MINPROB_CHECK, OnMinprobCheck)
	ON_CBN_SELCHANGE(IDC_TYPELOG_COMBO, OnSelchangeTypelogCombo)
	ON_BN_CLICKED(IDC_MOVED_CHECK, OnMovedCheck)
	ON_BN_CLICKED(IDC_DEAD_CHECK, OnDeadCheck)
	ON_BN_CLICKED(IDC_OCCUR_CHECK, OnOccurCheck)
	ON_BN_CLICKED(IDC_PROCESS_BUTTON, OnProcessButton)
	ON_BN_CLICKED(IDC_SPAWN_BUTTON, OnSpawnButton)
	ON_BN_CLICKED(IDC_GRID_BUTTON, OnGridButton)
	ON_BN_CLICKED(IDC_MOVING_CHECK, OnMovingCheck)
	ON_EN_CHANGE(IDC_MOVED_EDIT, OnChangeMovedEdit)
	ON_EN_CHANGE(IDC_DEAD_EDIT, OnChangeDeadEdit)
	ON_EN_CHANGE(IDC_OCCUR_EDIT, OnChangeOccurEdit)
	ON_EN_CHANGE(IDC_MINPROB_EDIT, OnChangeMinprobEdit)
	ON_EN_CHANGE(IDC_AROUND_EDIT, OnChangeAroundEdit)
	ON_WM_DESTROY()
	ON_COMMAND(ID_DATABASE_CONNECTION, OnDatabaseConnection)
	ON_COMMAND(ID_OUTPUT_SQLOPT, OnOutputSqlopt)
	ON_BN_CLICKED(IDC_WRITE_BUTTON, OnWriteButton)
	ON_BN_CLICKED(IDC_OUT_NPCS, OnOutNpcs)
	ON_BN_CLICKED(IDC_OUT_SPAWNS, OnOutSpawns)
	ON_BN_CLICKED(IDC_OUT_DOORS, OnOutDoors)
	ON_BN_CLICKED(IDC_OUT_TELEPORTS, OnOutTeleports)
	ON_BN_CLICKED(IDC_OUT_GRIDS, OnOutGrids)
	ON_BN_CLICKED(IDC_OUT_MERCHANTS, OnOutMerchants)
	ON_BN_CLICKED(IDC_LOADDB_BUTTON, OnLoaddbButton)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DATAMODE, OnDatamode)
	ON_EN_CHANGE(IDC_AFFIRM_EDIT, OnChangeAffirmEdit)
	ON_BN_CLICKED(IDC_AFFIRM_CHECK, OnAffirmCheck)
	ON_EN_CHANGE(IDC_CAMP_EDIT, OnChangeCampEdit)
	ON_BN_CLICKED(IDC_CAMP_CHECK, OnCampCheck)
	ON_BN_CLICKED(IDC_DELTAZ_CHECK, OnDeltazCheck)
	ON_EN_CHANGE(IDC_DELTAZ_EDIT, OnChangeDeltazEdit)
	ON_BN_CLICKED(IDC_ZONE_ZOOM_IN, OnZoneZoomIn)
	ON_BN_CLICKED(IDC_ZONE_ZOOM_OUT, OnZoneZoomOut)
	ON_BN_CLICKED(IDC_ZONE_RESET, OnZoneReset)
	ON_BN_CLICKED(IDC_ZONE_BIG, OnZoneBig)
	ON_BN_CLICKED(IDC_RELOAD_MAP, OnReloadMap)
	ON_BN_CLICKED(IDC_FACTIONS_BUTTON, OnFactionsButton)
	ON_BN_CLICKED(IDC_LOOT_BUTTON, OnLootButton)
	ON_BN_CLICKED(IDC_SPELLS_BUTTON, OnSpellsButton)
	ON_BN_CLICKED(IDC_GO_BUTTON, OnGoButton)
	ON_BN_CLICKED(IDC_SAVE_IDS, OnSaveIds)
	ON_BN_CLICKED(IDC_RESET_IDS, OnResetIds)
	ON_BN_CLICKED(IDC_LOAD_IDS, OnLoadIds)
	ON_BN_CLICKED(IDC_FIXED_MERGE, OnFixedMerge)
	ON_BN_CLICKED(IDC_PATHING_MERGE, OnPathingMerge)
	ON_EN_CHANGE(IDC_FIXED_MERGE_EDIT, OnChangeFixedMergeEdit)
	ON_EN_CHANGE(IDC_PATH_MERGE_EDIT, OnChangePathMergeEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEQBuilderDlg message handlers

BOOL CEQBuilderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// menu
	m_pMenu = new CMenu();
	m_pMenu->LoadMenu(IDR_MENU1);
	SetMenu( m_pMenu );

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// chemin
	char path[MAX_PATH]; 
	GetCurrentDirectory(MAX_PATH,path);
	chemin.Format( "%s", path );

	// ini file
	eqbini = new eqbuilderini(chemin);
	
	// database
	db = &db_real;
	dbconnectdlg = new DBConnect( eqbini->host, eqbini->user, eqbini->password, eqbini->db, this );
	db->Connexion( eqbini->host, eqbini->user, eqbini->password, eqbini->db );

	//try to load the saved ID settings from a file
	m_ids.ReadFromFile();


	// output options
	sqloptions = new SqlOptionsDlg(/*eqbini->useopt, eqbini->zoneid,eqbini->npcid, 
		eqbini->spawnid, eqbini->gridid, eqbini->sqldelete, eqbini->usedb, */
		eqbini->eqmaps_path, eqbini->eqemumaps_path,
		&m_ids);

	// 
	zones = NULL;
	compiledlogs = NULL;
	zonelogs = NULL;
	currentkeys = NULL;
	listNPCs = NULL;
	fixedMobs = NULL;
	fixedSpawns = NULL;
	gridSpawns = NULL;
	listGrids = NULL;
	listShops = NULL;
	doors = NULL;
	teleports = NULL;
	currentlog = NULL;

	out_npcs = false;
	out_spawns = false;
	out_doors = false;
	out_teleports = false;
	out_grids = false;
	out_merchants = false;

	cur_merchant = NULL;

	CString outputrep;
	outputrep.Format( "%s\\output", chemin  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString logrep;
	logrep.Format( "%s\\logs", chemin );

	if( _access( logrep, 0 ) == -1 ) {
		CreateDirectory( logrep, NULL );
	}

	// zones
	zonecombo = static_cast<CComboBox*>(GetDlgItem(IDC_ZONE_COMBO));
	if ( db->connected ) {
		zones = db->GetZones();
		for ( int i=0; i<zones->getsize(); i++ ) {
			CString p;
			p.Format("%s (%s)", (const char *) zones->get(i)->long_name, (const char *) zones->get(i)->short_name);
			zonecombo->AddString( p );
		}
		zonecombo->EnableWindow();
//zonecombo->SetCurSel(209);
	} else {
		zonecombo->EnableWindow( false );
	}


	// file dialog
	fileopendialog = new CFileDialog( true, NULL, NULL, OFN_HIDEREADONLY , "EQ Logs (*.txt,*.bf)|*.txt;*.bf|Old Text Logs (*.txt)|*.txt|Build Files (*.bf)|*.bf||", this );
//	fileopendialog = new CFileDialog( true, NULL, NULL, 0, "Old Text Logs (*.txt)|*.txt|Build Files (*.bf)|*.bf||", this );
	filesavedialog = new CFileDialog( false, "txt", "*.txt", NULL, NULL, this );
//	diropendialog

	bLoadDBButton = static_cast<CButton*>(GetDlgItem(IDC_LOADDB_BUTTON));
	
	//zone viewer:
	//vZoneView = static_cast<CZoneViewer*>(GetDlgItem(IDC_ZONEVIEW));
	m_bigZone = new BigZoneDlg();
	m_bigZone->SetDialog(this);
	text_map = NULL;
	bReloadMap = static_cast<CButton*>(GetDlgItem(IDC_RELOAD_MAP));

/*	//faction mapper
	m_factionDialog = new FactionsDialog();

	//loot mapper
	m_lootDialog = new LootDialog();*/

	//logs
	bAddLog = static_cast<CButton*>(GetDlgItem(IDC_ADD_BUTTON));
	bAddDir = static_cast<CButton*>(GetDlgItem(IDC_ADD_DIR_BUTTON));
	bCompileLog = static_cast<CButton*>(GetDlgItem(IDC_COMPILE_BUTTON));
	bCompileAll = static_cast<CButton*>(GetDlgItem(IDC_COMPILE_ALL_BUTTON));
	bDeleteLog = static_cast<CButton*>(GetDlgItem(IDC_DELETE_BUTTON));
	bDeleteAll = static_cast<CButton*>(GetDlgItem(IDC_DELETE_ALL_BUTTON));
	lLogList = static_cast<CTreeCtrl*>(GetDlgItem(IDC_LOG_TREE));
	cTypelogCombo = static_cast<CComboBox*>(GetDlgItem(IDC_TYPELOG_COMBO));
	cTypelogCombo->AddString( "NpcType" );
	cTypelogCombo->AddString( "Pathing" );
	cTypelogCombo->AddString( "Raid" );

	//scores
	sDoorStatic = static_cast<CStatic*>(GetDlgItem(IDC_DOOR_STATIC));
	sTeleportStatic = static_cast<CStatic*>(GetDlgItem(IDC_TELEPORT_STATIC));
	sNPCStatic = static_cast<CStatic*>(GetDlgItem(IDC_NPC_STATIC));
	sMobStatic = static_cast<CStatic*>(GetDlgItem(IDC_MOB_STATIC));
	sSpawnStatic = static_cast<CStatic*>(GetDlgItem(IDC_SPAWN_STATIC));
	sGridStatic = static_cast<CStatic*>(GetDlgItem(IDC_GRID_STATIC));
	sMerchantStatic = static_cast<CStatic*>(GetDlgItem(IDC_MERCHANT_STATIC));

	ResetScores();

	//lists
	lMobList = static_cast<CTreeCtrl*>(GetDlgItem(IDC_MOB_TREE));
	lSpawnList = static_cast<CTreeCtrl*>(GetDlgItem(IDC_SPAWN_TREE));
	bProcessButton = static_cast<CButton*>(GetDlgItem(IDC_PROCESS_BUTTON));
	
	lSpawnTabs = static_cast<CTabCtrl*>(GetDlgItem(IDC_DATAMODE));
	lSpawnTabs->InsertItem(0, "Spawn Groups");
	lSpawnTabs->InsertItem(1, "Merchants");


	//output

	outputtype = 1;

	bWriteButton = static_cast<CButton*>(GetDlgItem(IDC_WRITE_BUTTON));
	bSQL = static_cast<CButton*>(GetDlgItem(IDC_SQL_RADIO));
	bSQL->SetCheck( true );
	bDatabase = static_cast<CButton*>(GetDlgItem(IDC_DATABASE_RADIO));
	bDatabase->SetCheck( false );


	// filters
	eAroundEdit = static_cast<CEdit*>(GetDlgItem(IDC_AROUND_EDIT));
	bAroundCheck = static_cast<CButton*>(GetDlgItem(IDC_AROUND_CHECK));
	bAroundCheck->SetCheck( true );
	eAroundEdit->EnableWindow();
	filtres.coord_error = 4;
	eAroundEdit->SetWindowText( "4" );
	vZoneView.SetMinSpawnRadius(filtres.coord_error);
	m_bigZone->SetMinSpawnRadius(filtres.coord_error);

	eCampEdit = static_cast<CEdit*>(GetDlgItem(IDC_CAMP_EDIT));
	bCampCheck = static_cast<CButton*>(GetDlgItem(IDC_CAMP_CHECK));
	bCampCheck->SetCheck( true );
	eCampEdit->EnableWindow();
	filtres.coord_error = 5;
	eCampEdit->SetWindowText( "5" );

	eDeltazEdit = static_cast<CEdit*>(GetDlgItem(IDC_DELTAZ_EDIT));
	bDeltazCheck = static_cast<CButton*>(GetDlgItem(IDC_DELTAZ_CHECK));
	bDeltazCheck->SetCheck( true );
	eDeltazEdit->EnableWindow();
	filtres.deltaz = 15;
	eDeltazEdit->SetWindowText( "15" );

	eMinprobEdit = static_cast<CEdit*>(GetDlgItem(IDC_MINPROB_EDIT));
	bMinprobCheck = static_cast<CButton*>(GetDlgItem(IDC_MINPROB_CHECK));
	bMinprobCheck->SetCheck( true );
	eMinprobEdit->EnableWindow();
	filtres.minprob = "50";
	eMinprobEdit->SetWindowText( filtres.minprob );

	eOccurEdit = static_cast<CEdit*>(GetDlgItem(IDC_OCCUR_EDIT));
	bOccurCheck = static_cast<CButton*>(GetDlgItem(IDC_OCCUR_CHECK));
	bOccurCheck->SetCheck( true );
	eOccurEdit->EnableWindow();
	filtres.occur = "5";
	eOccurEdit->SetWindowText( filtres.occur );

	eMovedEdit = static_cast<CEdit*>(GetDlgItem(IDC_MOVED_EDIT));
	bMovedCheck = static_cast<CButton*>(GetDlgItem(IDC_MOVED_CHECK));
	bMovedCheck->SetCheck( true );
	eMovedEdit->EnableWindow();
	filtres.moved = "5";
	eMovedEdit->SetWindowText( filtres.moved );

	eAffirmEdit = static_cast<CEdit*>(GetDlgItem(IDC_AFFIRM_EDIT));
	bAffirmCheck = static_cast<CButton*>(GetDlgItem(IDC_AFFIRM_CHECK));
	bAffirmCheck->SetCheck( true );
	eAffirmEdit->EnableWindow();
	filtres.affirm = 10;
	eAffirmEdit->SetWindowText( "10" );

	eDeadEdit = static_cast<CEdit*>(GetDlgItem(IDC_DEAD_EDIT));
	bDeadCheck = static_cast<CButton*>(GetDlgItem(IDC_DEAD_CHECK));
	bDeadCheck->SetCheck( true );
	eDeadEdit->EnableWindow();
	filtres.dead = "5";
	eDeadEdit->SetWindowText( filtres.dead );

	CString p;
	ePathingErrEdit = static_cast<CEdit*>(GetDlgItem(IDC_PATH_MERGE_EDIT));
	bPathingErrCheck = static_cast<CButton*>(GetDlgItem(IDC_PATHING_MERGE));
	bPathingErrCheck->SetCheck( true );
	ePathingErrEdit->EnableWindow();
	filtres.path_error = 8;
	p.Format("%.1f", filtres.path_error);
	ePathingErrEdit->SetWindowText( p );

	eFixedMergeErrEdit = static_cast<CEdit*>(GetDlgItem(IDC_FIXED_MERGE_EDIT));
	bFixedMergeErrCheck = static_cast<CButton*>(GetDlgItem(IDC_FIXED_MERGE));
	bFixedMergeErrCheck->SetCheck( true );
	eFixedMergeErrEdit->EnableWindow();
	filtres.fixedmerge_error = 8;
	p.Format("%.1f", filtres.fixedmerge_error);
	eFixedMergeErrEdit->SetWindowText( p );

	bMovingCheck = static_cast<CButton*>(GetDlgItem( IDC_MOVING_CHECK ));
	bMovingCheck->SetCheck( true );
	filtres.movementok = true;

	//bar de progression
	pProgress = static_cast<CProgressCtrl*>(GetDlgItem(IDC_PROGRESS));
	pProgress->SetRange( 0, 100 );
	pProgress->SetPos(0);
	m_progressText.SetWindowText("Idle");
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEQBuilderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEQBuilderDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CEQBuilderDlg::ClearBuildResults() {
	if(listNPCs != NULL) {
		delete listNPCs;
		listNPCs = NULL;
	}

	if(listGrids != NULL) {
		delete listGrids;
		listGrids = NULL;
	}

	if(doors != NULL) {
		delete doors;
		doors = NULL;
	}

	if(teleports != NULL) {
		delete teleports;
		teleports = NULL;
	}

	if(fixedMobs != NULL) {
		delete fixedMobs;
		fixedMobs = NULL;
	}

	if(fixedSpawns != NULL) {
		delete fixedSpawns;
		fixedSpawns = NULL;
	}

	if(gridSpawns != NULL) {
		delete gridSpawns;
		gridSpawns = NULL;
	}

	if(listShops != NULL) {
		delete listShops;
		listShops = NULL;
	}
	cur_merchant = NULL;
}

void CEQBuilderDlg::ReleaseMemory() {

/*	commented to try to fix crashes... I dont wanna hear it

	if(zonelogs != NULL) {
		delete zonelogs;
		zonelogs = NULL;
	}

	if(compiledlogs != NULL) {
		delete compiledlogs;
		compiledlogs = NULL;
	}
*/
	currentlog = NULL;

	if(currentkeys != NULL) {
		delete currentkeys;
		currentkeys = NULL;
	}

	ClearBuildResults();

	
	vZoneView.ClearSpawnLists();
	m_bigZone->ClearLists();

	db->InvalidateCaches();
}

void CEQBuilderDlg::OnSelchangeZoneCombo() 
{
}


void CEQBuilderDlg::OnCloseupZoneCombo() 
{
}

void CEQBuilderDlg::OnGoButton() 
{

	if(zones->get(zonecombo->GetCurSel()) == NULL)
		return;
	
	// spawns
	zoneid = zones->get(zonecombo->GetCurSel())->id;
	zonename = zones->get(zonecombo->GetCurSel())->short_name;

	// logs
	lLogList->DeleteAllItems();
	pProgress->SetPos( 0 );

	// memory
	ReleaseMemory();

	zonelogs = db->GetLogs( zonename );

	nblogs = zonelogs->getsize();

	compiledlogs = new log_list();

	for( int i=0;i<nblogs;i++ ) {
		// treeview
		CString logname;
		logname.Format( "%s", zonelogs->get(i)->name );
		CString logpath;
		logpath.Format( "%s\\logs\\%s\\%s", chemin, zonename, logname );
		if( _access( logpath, 0 ) == -1 ) {
			continue;
		}
		lLogList->InsertItem( logname );
		
		// structures

		clog* log = new clog();
		log->name = logname;
		log->type = zonelogs->get(i)->type;
		
		if(logname.Find(".bf") != -1) {
			compiledlogs->bf_add( log );
		} else {
			compiledlogs->txt_add( log );
		}

	}

	// scores
	ResetScores();

	// data
	lMobList->DeleteAllItems();
	lSpawnList->DeleteAllItems();

	//ID gens
	m_ids.SetZoneID(zoneid);
	m_ids.Reset();

	// buttons
	bAddLog->EnableWindow();
	bAddDir->EnableWindow();
	bDeleteLog->EnableWindow();
	bDeleteAll->EnableWindow();
	bCompileAll->EnableWindow();
	bCompileLog->EnableWindow(false);
	bWriteButton->EnableWindow(false);
	cTypelogCombo->EnableWindow(false);
	bProcessButton->EnableWindow(false);
	bLoadDBButton->EnableWindow(false);
	bReloadMap->EnableWindow();

	ReloadMap();

}

void CEQBuilderDlg::ResetScores() {
	sDoorStatic->SetWindowText( "0" );
	sTeleportStatic->SetWindowText( "0" );
	sNPCStatic->SetWindowText( "0" );
	sMobStatic->SetWindowText( "0" );
	sSpawnStatic->SetWindowText( "0f,0g" );
	sGridStatic->SetWindowText( "0" );
	sMerchantStatic->SetWindowText( "0" );
}

void CEQBuilderDlg::AddLogFile(CString pathname, CString filename, CString fileext) {
	CString destname;
	CString filetitle;

	bool alreadyexists = false;

	destname.Format( "%s\\logs\\%s\\%s", chemin, zonename, filename );

	if ( fileext == "txt" || fileext == ".txt" ) {

		if( _access( destname, 0 ) == -1 ) {

			CString zonelog;
			zonelog.Format( "%s\\logs\\%s", chemin, zonename );
			if ( _access( zonelog, 0 ) == -1 ) {
				CreateDirectory( zonelog, NULL );					
			}
			if ( !CopyFile( pathname, destname, alreadyexists ) ) {
				return;
			}
		} else {
			if ( db->logexists( zonename, filename ) ) {
				return;
			}
		}

		// treeview
		lLogList->InsertItem( filename );
		db->SaveLog( zonename, filename );

		// structures
		nblogs ++;
		clog* log = new clog();
		log->name = filename;
		log->type = logPathing;
		compiledlogs->txt_add( log );

	} else if(fileext == "bf" || fileext == ".bf") {
	
		if( _access( destname, 0 ) == -1 ) {

			CString zonelog;
			zonelog.Format( "%s\\logs\\%s", chemin, zonename );
			if ( _access( zonelog, 0 ) == -1 ) {
				CreateDirectory( zonelog, NULL );					
			}
			if ( !CopyFile( pathname, destname, alreadyexists ) ) {
				return;
			}
		} else {
			if ( db->logexists( zonename, filename ) ) {
				return;
			}
		}

		// treeview
		lLogList->InsertItem( filename );
		db->SaveLog( zonename, filename );

		// structures
		nblogs ++;
		clog* log = new clog();
		log->name = filename;
		compiledlogs->bf_add( log );
	}

}

void CEQBuilderDlg::OnAddButton() 
{

//	fileopendialog->m_ofn.lpstrFilter = "text files";
//	fileopendialog->m_ofn.lpstrFilter = "EQ Logs (*.txt,*.bf)";
//	fileopendialog->m_ofn.lpstrFilter = "EQ Logs (*.txt,*.bf)|*.txt;*.bf|Old Text Logs (*.txt)|*.txt|Build Files (*.bf)|*.bf";

	if ( fileopendialog->DoModal() == IDOK ) {

		CString pathname;
		CString filename;
		CString fileext;

		pathname = fileopendialog->GetPathName();
		filename = fileopendialog->GetFileName();
		fileext = fileopendialog->GetFileExt();

		AddLogFile(pathname, filename, fileext);
	}


}

void CEQBuilderDlg::OnAddDirButton() {
  if(BrowseForDirectory.Browse("Select folder containing logs to add (not recursive)...")) {
    CString Folder(BrowseForDirectory.GetFolderPath ());
	CString OldLogs(Folder);
	CString NewLogs(Folder);
	OldLogs += "*.txt";
	NewLogs += "*.bf";
	
	AddFilesMatching(OldLogs);
	AddFilesMatching(NewLogs);
  }
}

void CEQBuilderDlg::AddFilesMatching(const CString &match) {
	CFileFind finder;
	CPathSplit split;

	BOOL bWorking = finder.FindFile(match);
	while (bWorking) {
		bWorking = finder.FindNextFile();
		if (finder.IsDirectory()) {
			if (finder.IsDots())
				continue;
			//directory... recurse?
		} else {
			split.Split(finder.GetFilePath());
			CString path = finder.GetFilePath();
			CString extension = split.GetExtension();
			CString file = finder.GetFileName();
			AddLogFile(path, file, extension);
		}
	}
}

void CEQBuilderDlg::OnDeleteAllButton() 
{
	db->ClearLogs(zonename);

	//TODO: should clear out the logs directory maybe?


	// logs
	lLogList->DeleteAllItems();
	pProgress->SetPos( 0 );

	// memory
	ReleaseMemory();

	currentlog = NULL;
	
	nblogs = 0;
	zonelogs = new log_list();
	compiledlogs = new log_list();

	// scores
	ResetScores();

	// data
	lMobList->DeleteAllItems();
	lSpawnList->DeleteAllItems();

	// buttons
	bAddLog->EnableWindow();
	bAddDir->EnableWindow();
	bDeleteAll->EnableWindow();
	bCompileAll->EnableWindow();
	bDeleteLog->EnableWindow(false);
	bCompileLog->EnableWindow(false);
	bWriteButton->EnableWindow(false);
	cTypelogCombo->EnableWindow(false);
	bProcessButton->EnableWindow(false);
	bLoadDBButton->EnableWindow(false);
}

void CEQBuilderDlg::OnDeleteButton() 
{
	//cannot implement this until we change log_list into STL containers
	//DeleteFile
}

void CEQBuilderDlg::OnSelchangedLogTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// log
	CString filename;
	filename = lLogList->GetItemText( lLogList->GetSelectedItem() );

	CString logpath;
	logpath.Format( "%s\\logs\\%s\\%s", chemin, zonename, filename );

	selectedlog = 0;
	for ( int i = 0; i<nblogs; i++ ) {
		if ( compiledlogs->get(i)->name == filename ) {
			selectedlog = i;
		}
	}

	currentlog = compiledlogs->get(selectedlog);

	// type log
	cTypelogCombo->SetCurSel( currentlog->type );

	pProgress->SetPos( currentlog->compilepos );

	// boutons
	if ( currentlog->compiled ) {
		bCompileLog->EnableWindow(false);
		cTypelogCombo->EnableWindow(false);
	} else {
		bCompileLog->EnableWindow();
		cTypelogCombo->EnableWindow();
	}
	bDeleteLog->EnableWindow();

}

void CEQBuilderDlg::OnCompileButton() 
{
	// treeview
	lMobList->DeleteAllItems();
	lSpawnList->DeleteAllItems();
	currentlog->compilepos = 0;
	pProgress->SetPos( 0 );
	
	if(currentlog->mobadd == NULL)
		currentlog->mobadd = new mob_list();
	if(currentlog->mobinit == NULL)
		currentlog->mobinit = new mob_list();

	m_progressText.SetWindowText("Compiling "+currentlog->name);
	m_progressText.RedrawWindow();
	// parsing
	if(currentlog->is_build_file) {
		load_build_file();
	} else {
		parselog();
	}
	currentlog->compiled = true;

	// data
	ProcessData();

	// boutons
	bCompileLog->EnableWindow(false);
	cTypelogCombo->EnableWindow(false);
	bProcessButton->EnableWindow();

}

void CEQBuilderDlg::OnCompileAllButton() 
{
	// treeview
	lMobList->DeleteAllItems();
	lSpawnList->DeleteAllItems();
	pProgress->SetPos( 0 );


	clog *save = currentlog;
	int r;
	for(r = 0; r < nblogs; r++) {
		selectedlog = r;
		currentlog = compiledlogs->get(r);

		if(currentlog->compiled)
			continue;
		
		m_progressText.SetWindowText("Compiling "+currentlog->name);
		m_progressText.RedrawWindow();

		currentlog->compilepos = 0;
		if(currentlog->is_build_file) {
			load_build_file();
		} else {
			parselog();
		}
		currentlog->compiled = true;
	}

	// data
	ProcessData();

	// boutons
	bCompileLog->EnableWindow(false);
	cTypelogCombo->EnableWindow(false);
	bProcessButton->EnableWindow();
	
	currentlog = save;
}

void CEQBuilderDlg::OnDoorButton() 
{
	if ( nbdoors() == 0 ) {
		return;
	}

	
	if ( outputtype == 1 ) {
		bool delqueryok = false;
		if ( sqloptions->m_sqldelete == TRUE ) {
			delqueryok = true;
		}
		db->extractdoors( doors, teleports, chemin, zonename, delqueryok );
	}

}

void CEQBuilderDlg::OnTeleportButton() 
{
	if ( nbteleports() == 0 ) {
		return;
	}
	
	if ( outputtype == 1 ) {
		bool delqueryok = false;
		if ( sqloptions->m_sqldelete == TRUE ) {
			delqueryok = true;
		}
		db->extractteleports( teleports, chemin, zonename, delqueryok );
	}
}

void CEQBuilderDlg::OnNpcButton() 
{
	if ( nbnpcs() == 0 ) {
		return;
	}
	
	if ( outputtype == 1 ) {
		bool delqueryok = ( sqloptions->m_sqldelete == TRUE );
		bool usedb = ( sqloptions->m_usedb == TRUE );
		db->extractnpcs( listNPCs, chemin, zonename, delqueryok, usedb );
	}	
}


void CEQBuilderDlg::OnSpawnButton() 
{
	if ( nbvalidspawns() == 0 ) {
		return;
	}

	if ( outputtype == 1 ) {
		bool delqueryok = false;
		if ( sqloptions->m_sqldelete == TRUE ) {
			delqueryok = true;
		}
		db->extractspawns( fixedSpawns, gridSpawns, chemin, zonename, delqueryok );
	}	
	
}

void CEQBuilderDlg::OnGridButton() 
{
	if ( nbgrids() == 0 ) {
		return;
	}
	if ( outputtype == 1 ) {
		bool delqueryok = false;
		if ( sqloptions->m_sqldelete == TRUE ) {
			delqueryok = true;
		}
		db->extractgrids( listGrids, chemin, zonename, zoneid, delqueryok );
	}	

}

void CEQBuilderDlg::OnMerchantButton() 
{
	if ( nbmerchants() == 0 ) {
		return;
	}
	if ( outputtype == 1 ) {
		bool delqueryok = false;
		if ( sqloptions->m_sqldelete == TRUE ) {
			delqueryok = true;
		}
		db->extractmerchants( listShops, chemin, zonename, delqueryok );
	}	

}

void CEQBuilderDlg::OnSqlRadio() 
{
	outputtype = 1;
	bDatabase->SetCheck( false );
}

void CEQBuilderDlg::OnDatabaseRadio() 
{
	outputtype = 2;
	bSQL->SetCheck( false );
}

void CEQBuilderDlg::OnAroundCheck() 
{
	if ( bAroundCheck->GetCheck() ) {
		eAroundEdit->EnableWindow();
	} else {
		eAroundEdit->EnableWindow(false);
		filtres.coord_error = 10;
	}

}

void CEQBuilderDlg::OnMinprobCheck() 
{
	if ( bMinprobCheck->GetCheck() ) {
		eMinprobEdit->EnableWindow();
	} else {
		eMinprobEdit->EnableWindow(false);
		filtres.minprob = "50";
	}
}

void CEQBuilderDlg::OnSelchangeTypelogCombo() 
{
	switch(cTypelogCombo->GetCurSel()) {
	case 0:
		currentlog->type = logNPCType;
		break;
	case 1:
		currentlog->type = logPathing;
		break;
	case 2:
		currentlog->type = logRaid;
		break;
	default:
		//error... unknown type
		return;
	}
	db->UpdateLog( zonename, currentlog );

}

void CEQBuilderDlg::OnMovedCheck() 
{

	if ( bMovedCheck->GetCheck() ) {
		eMovedEdit->EnableWindow();
		OnChangeMovedEdit();
	} else {
		eMovedEdit->EnableWindow(false);
		filtres.moved = "5";
	}
	
}

void CEQBuilderDlg::OnDeadCheck() 
{
	if ( bDeadCheck->GetCheck() ) {
		eDeadEdit->EnableWindow();
		OnChangeDeadEdit();
	} else {
		eDeadEdit->EnableWindow(false);
		filtres.dead = "5";
	}
	
}

void CEQBuilderDlg::OnOccurCheck() 
{
	if ( bOccurCheck->GetCheck() ) {
		eOccurEdit->EnableWindow();
	} else {
		eOccurEdit->EnableWindow(false);
		filtres.occur = "5";
	}
		
}

void CEQBuilderDlg::OnAffirmCheck() 
{
	if ( bAffirmCheck->GetCheck() ) {
		eAffirmEdit->EnableWindow();
		OnChangeOccurEdit();
	} else {
		eAffirmEdit->EnableWindow(false);
		filtres.affirm = 10;
	}
}

void CEQBuilderDlg::OnCampCheck() 
{
	if ( bAffirmCheck->GetCheck() ) {
		eAffirmEdit->EnableWindow();
		OnChangeCampEdit();
	} else {
		eAffirmEdit->EnableWindow(false);
		filtres.camp_range = 0;
	}
}

void CEQBuilderDlg::OnDeltazCheck() 
{
	if ( bDeltazCheck->GetCheck() ) {
		eDeltazEdit->EnableWindow();
		OnChangeDeltazEdit();
	} else {
		eDeltazEdit->EnableWindow(false);
		filtres.deltaz = 999999;
	}
}

void CEQBuilderDlg::OnFixedMerge() 
{
	if ( bFixedMergeErrCheck->GetCheck() ) {
		eFixedMergeErrEdit->EnableWindow();
		OnChangeFixedMergeEdit();
	} else {
		eFixedMergeErrEdit->EnableWindow(false);
		filtres.fixedmerge_error = 0;
	}
}

void CEQBuilderDlg::OnPathingMerge() 
{
	if ( bPathingErrCheck->GetCheck() ) {
		ePathingErrEdit->EnableWindow();
		OnChangePathMergeEdit();
	} else {
		ePathingErrEdit->EnableWindow(false);
		filtres.path_error = 0;
	}
}

void CEQBuilderDlg::OnProcessButton() 
{
	ProcessData();	
}

void CEQBuilderDlg::OnMovingCheck() 
{
	if ( bMovingCheck->GetCheck() ) {
		filtres.movementok = true;
	} else {
		filtres.movementok = false;
	}
	
}

void CEQBuilderDlg::OnChangeFixedMergeEdit() 
{
	if ( bFixedMergeErrCheck->GetCheck() ) {
		CString p;
		eFixedMergeErrEdit->GetWindowText( p );
		filtres.fixedmerge_error = atof((const char *) p);
	}
}

void CEQBuilderDlg::OnChangePathMergeEdit() 
{
	if ( bPathingErrCheck->GetCheck() ) {
		CString p;
		ePathingErrEdit->GetWindowText( p );	
		filtres.path_error = atof((const char *) p);
	}
}

void CEQBuilderDlg::OnChangeMovedEdit() 
{
	if ( bMovedCheck->GetCheck() ) {
		eMovedEdit->GetWindowText( filtres.moved );		
	}
}

void CEQBuilderDlg::OnChangeDeadEdit() 
{
	if ( bDeadCheck->GetCheck() ) {
		eDeadEdit->GetWindowText( filtres.dead );		
	}
}

void CEQBuilderDlg::OnChangeOccurEdit() 
{
	if ( bOccurCheck->GetCheck() ) {
		eOccurEdit->GetWindowText( filtres.occur );		
	}	
}

void CEQBuilderDlg::OnChangeMinprobEdit() 
{
	if ( bMinprobCheck->GetCheck() ) {
		eMinprobEdit->GetWindowText( filtres.minprob );		
	}	
}

void CEQBuilderDlg::OnChangeAroundEdit() 
{
	if ( bAroundCheck->GetCheck() ) {
		CString into;
		eAroundEdit->GetWindowText( into );
		filtres.coord_error = atof(into);
		vZoneView.SetMinSpawnRadius(filtres.coord_error);
		m_bigZone->SetMinSpawnRadius(filtres.coord_error);
	} else {
		filtres.coord_error = 10;		//default
	}
}

void CEQBuilderDlg::OnChangeAffirmEdit() 
{
	if ( bAffirmCheck->GetCheck() ) {
		CString into;
		eAffirmEdit->GetWindowText( into );
		filtres.affirm = atoi(into);
	} else {
		filtres.affirm = 10;		//default
	}
}

void CEQBuilderDlg::OnChangeCampEdit() 
{
	if ( bCampCheck->GetCheck() ) {
		CString into;
		eCampEdit->GetWindowText( into );
		filtres.camp_range = atof(into);
	} else {
		filtres.camp_range = 0;		//default on disabled
	}
}

void CEQBuilderDlg::OnChangeDeltazEdit() 
{
	if ( bDeltazCheck->GetCheck() ) {
		CString into;
		eDeltazEdit->GetWindowText( into );
		filtres.deltaz = atof(into);
	} else {
		filtres.deltaz = 99999;		//default on disabled
	}
}

void CEQBuilderDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	//seem to be getting this message in a strange place, screw cleaning up
	delete zones;

	delete fileopendialog;
	delete filesavedialog;

	ReleaseMemory();

}

void CEQBuilderDlg::OnDatabaseConnection() 
{
	if ( dbconnectdlg->DoModal() == IDOK ) {
		// nothing has changed
		if ( ( eqbini->host == dbconnectdlg->m_host ) &&
			 ( eqbini->user == dbconnectdlg->m_user ) &&
			 ( eqbini->password == dbconnectdlg->m_password ) &&
			 ( eqbini->db == dbconnectdlg->m_database ) ) {
			return;
		}

		// database already connected
		if ( db->connected ) {
			delete [] db;
			for ( int i=0; i<zonecombo->GetCount(); i++ ) {
				zonecombo->DeleteString(0);
			}
			delete [] zones;
			db = new database();
		}

		// saving ini file
		eqbini->setdbparams( dbconnectdlg->m_host, dbconnectdlg->m_user, dbconnectdlg->m_password, dbconnectdlg->m_database );

		// connect to the db with the new db parameters
		db->Connexion( eqbini->host, eqbini->user, eqbini->password, eqbini->db );
		
		// connection success
		if ( db->connected ) {
			zones = db->GetZones();
			for ( int i=0; i<zones->getsize(); i++ ) {
				CString p;
				p.Format("%s (%s)", (const char *) zones->get(i)->long_name, (const char *) zones->get(i)->short_name);
				zonecombo->AddString( p );
			}
			zonecombo->EnableWindow();
			// not successfull
		} else {
			zonecombo->EnableWindow( false );
		}
	}
	
}

void CEQBuilderDlg::OnOutputSqlopt() 
{
	if ( sqloptions->DoModal() == IDOK ) {

//		eqbini->setsqlparams( sqloptions->m_useopt, sqloptions->m_zoneid, sqloptions->m_npcid, sqloptions->m_spawnid, sqloptions->m_gridid, (sqloptions->m_sqldelete==1)?true:false, (sqloptions->m_usedb==1)?true:false );
		eqbini->setgeneralparams( sqloptions->m_EQMaps, sqloptions->m_EQEmuMaps );
		
	}
}

void CEQBuilderDlg::OnWriteButton() 
{
	if(out_npcs)
		OnNpcButton();
	if(out_doors)
		OnDoorButton();
	if(out_teleports)
		OnTeleportButton();
	if(out_spawns)
		OnSpawnButton();
	if(out_grids)
		OnGridButton();
	if(out_merchants)
		OnMerchantButton();
	if ( outputtype == 1 ) {
		db->extractdeletes( chemin, zonename, true );
	}
	
	m_ids.DataWritten();
}

void CEQBuilderDlg::OnOutNpcs() 
{
	out_npcs = !out_npcs;
}

void CEQBuilderDlg::OnOutSpawns() 
{
	out_spawns = !out_spawns;
}

void CEQBuilderDlg::OnOutDoors() 
{
	out_doors = !out_doors;
}

void CEQBuilderDlg::OnOutTeleports() 
{
	out_teleports = !out_teleports;
}

void CEQBuilderDlg::OnOutGrids() 
{
	out_grids = !out_grids;
}

void CEQBuilderDlg::OnOutMerchants() 
{
	out_merchants = !out_merchants;
}

void CEQBuilderDlg::OnLoaddbButton() 
{
	bLoadDBButton->EnableWindow(true);
	db->loadZone(zoneid, zonename, this);
}

void CEQBuilderDlg::OnDatamode(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetDataDisplay(lSpawnTabs->GetCurSel());
	*pResult = 0;
}



void CEQBuilderDlg::OnZoneZoomIn() 
{
	vZoneView.ZoomIn();
}

void CEQBuilderDlg::OnZoneZoomOut() 
{
	vZoneView.ZoomOut();
}

void CEQBuilderDlg::OnZoneReset() 
{
	vZoneView.ResetView();
}

void CEQBuilderDlg::OnZoneBig() 
{
	if ( m_bigZone->DoModal() == IDOK ) {
	}
}

void CEQBuilderDlg::OnReloadMap() 
{
	ReloadMap();
}

void CEQBuilderDlg::OnFactionsButton() 
{
/*	m_factionDialog->UpdateLists(listNPCs);
	if ( m_factionDialog->DoModal() == IDOK ) {
	}
*/
}

void CEQBuilderDlg::OnLootButton() 
{
/*	m_lootDialog->UpdateLists(listNPCs);
	if ( m_lootDialog->DoModal() == IDOK ) {
	}
*/
}

void CEQBuilderDlg::OnSpellsButton() 
{
	// TODO: Add your control notification handler code here
	
}


void CEQBuilderDlg::OnSaveIds() 
{
	m_ids.SaveToFile();
}


void CEQBuilderDlg::OnResetIds() 
{
	m_ids.Reset();
}


void CEQBuilderDlg::OnLoadIds() 
{
	m_ids.ReadFromFile();
}











