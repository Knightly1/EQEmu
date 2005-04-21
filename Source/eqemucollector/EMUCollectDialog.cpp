// EMUCollectDialog.cpp : implementation file
//

#include "stdafx.h"
#include "EMUCollect.h"
#include "EMUCollectDialog.h"
#include "pcvars.h"
#include "../common/packetfile.h"
#include "PrivacyDialog.h"
#include "MobInfoDlg.h"
#include "ConfirmDialog.h"
#include "PacketHandler.h"
#include "../common/MiscFunctions.h"
#include "mobinfo_handler.h"
#include <afxtempl.h>
#include <process.h>    

//functions defined in our statically linked plugins
extern void SetupPacketFileCollector(HandlerCallbacks *calls);
extern void SetupPrivacyPacketFileCollector(HandlerCallbacks *calls, const char *priv_name);
extern void SetupPacketHeaderHandler(HandlerCallbacks *calls, bool clean);


bool CollectMobInfo = false;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bool LookupPcapDevice(int id);
extern void GetDeviceList(CArray<CString, CString&> &);
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EMUCollectDialog dialog

EMUCollectDialog::EMUCollectDialog(CWnd* pParent /*=NULL*/)
	: CDialog(EMUCollectDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(EMUCollectDialog)
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	dotextlogs = false;
	dobinlogs = false;

	filterMode = FILTER_NONE;
}

void EMUCollectDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(EMUCollectDialog)
	DDX_Control(pDX, IDC_ForceFlush, m_ForceFlush);
	DDX_Control(pDX, IDC_ONMOBINFO, m_OnMobInfo);
	DDX_Control(pDX, ID_MOBINFO, m_MobInfo);
	DDX_Control(pDX, IDC_StopCollecting, m_StopButton);
	DDX_Control(pDX, IDC_PrivacyLabel, m_PrivacyLabel);
	DDX_Control(pDX, IDC_PrivacyName, m_PrivacyName);
	DDX_Control(pDX, IDC_PrivacyMode, m_PrivacyMode);
	DDX_Control(pDX, IDC_IgnoreWorld, m_IgnoreWorld);
	DDX_Control(pDX, IDC_DEBUG_CONSOLE, m_DebugConsole);
	DDX_Control(pDX, IDC_STAT_ITEMS, m_StatsItems);
	DDX_Control(pDX, IDC_STAT_PACKETS, m_StatsPackets);
	DDX_Control(pDX, IDC_DEVICE, m_Device);
	DDX_Control(pDX, IDC_STAT_RUNNING, m_StatsRunning);
	DDX_Control(pDX, IDC_STAT_OPCODES, m_StatsOpcodes);
	DDX_Control(pDX, IDC_RawLogging, m_RawLogging);
	DDX_Control(pDX, IDC_ShowOpcodes, m_ShowOpcodes);
	DDX_Control(pDX, IDC_UPLOAD_LABEL, m_UploadLabel);
	DDX_Control(pDX, IDC_WATCH_IP, m_WatchIP);
	DDX_Control(pDX, IDC_UPLOAD_NAME, m_UploadName);
	DDX_Control(pDX, IDC_TEXTLOGS, m_TextLogs);
	DDX_Control(pDX, IDC_TEXTFILE, m_TextFile);
	DDX_Control(pDX, IDC_BINLOGS, m_BinLogs);
	DDX_Control(pDX, IDC_BINFILE, m_BinFile);
	DDX_Control(pDX, IDC_DBUpload, m_DBUpload);
	DDX_Control(pDX, IDC_CollectZones, m_CollectZones);
	DDX_Control(pDX, IDC_CollectSpawns, m_CollectSpawns);
	DDX_Control(pDX, IDC_CollectObjects, m_CollectObjects);
	DDX_Control(pDX, IDC_CollectItems, m_CollectItems);
	DDX_Control(pDX, IDC_CollectGrids, m_CollectGrids);
	DDX_Control(pDX, IDC_CollectDoors, m_CollectDoors);
	DDX_Control(pDX, IDC_BeginCollecting, m_StartButton);
	DDX_Control(pDX, IDC_SC_GROUP, m_SC_group);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(EMUCollectDialog, CDialog)
	//{{AFX_MSG_MAP(EMUCollectDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_DBUpload, OnDBUploadCheck)
	ON_BN_CLICKED(IDC_ShowOpcodes, OnShowOpcodes)
	ON_BN_CLICKED(IDC_CollectItems, OnCollectItems)
	ON_BN_CLICKED(IDC_CollectDoors, OnCollectDoors)
	ON_BN_CLICKED(IDC_CollectObjects, OnCollectObjects)
	ON_BN_CLICKED(IDC_CollectZones, OnCollectZones)
	ON_BN_CLICKED(IDC_CollectSpawns, OnCollectSpawns)
	ON_BN_CLICKED(IDC_CollectGrids, OnCollectGrids)
	ON_BN_CLICKED(IDC_TEXTLOGS, OnTextlogs)
	ON_BN_CLICKED(IDC_BINLOGS, OnBinlogs)
	ON_BN_CLICKED(IDC_SelectTextLog, OnSelectTextLog)
	ON_BN_CLICKED(IDC_SelectBinLog, OnSelectBinLog)
	ON_BN_CLICKED(IDC_FilterNone, OnFilterNone)
	ON_BN_CLICKED(IDC_FilterInclude, OnFilterInclude)
	ON_BN_CLICKED(IDC_OPI_ADD, OnOpiAdd)
	ON_BN_CLICKED(IDC_OPI_REMOVE, OnOpiRemove)
	ON_BN_CLICKED(IDC_OPI_LOAD, OnOpiLoad)
	ON_BN_CLICKED(IDC_OPI_SAVE, OnOpiSave)
	ON_BN_CLICKED(IDC_OPE_ADD, OnOpeAdd)
	ON_BN_CLICKED(IDC_OPE_REMOVE, OnOpeRemove)
	ON_BN_CLICKED(IDC_OPE_LOAD, OnOpeLoad)
	ON_BN_CLICKED(IDC_OPE_SAVE, OnOpeSave)
	ON_BN_CLICKED(IDC_BeginCollecting, OnBeginCollecting)
	ON_EN_CHANGE(IDC_WATCH_IP, OnChangeWatchIp)
	ON_BN_CLICKED(IDC_ITEMSONLY, OnItemsonly)
	ON_BN_CLICKED(IDC_RawLogging, OnRawLogging)
	ON_BN_CLICKED(IDC_ALLPACKETS, OnAllpackets)
	ON_BN_CLICKED(IDC_StopCollecting, OnStopCollecting)
	ON_BN_CLICKED(IDC_DEBUG_CONSOLE, OnDebugConsole)
	ON_BN_CLICKED(IDC_ABOUTPRIVACY, OnAboutPrivacy)
	ON_BN_CLICKED(IDC_IgnoreWorld, OnIgnoreWorld)
	ON_BN_CLICKED(IDC_PrivacyMode, OnPrivacyMode)
	ON_BN_CLICKED(IDC_SaveSettings, OnSaveSettings)
	ON_BN_CLICKED(ID_MOBINFO, OnMobInfo)
	ON_BN_CLICKED(IDC_ONMOBINFO, OnOnMobInfo)
	ON_BN_CLICKED(IDC_ForceFlush, OnForceFlush)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EMUCollectDialog message handlers


void EMUCollectDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void EMUCollectDialog::OnPaint() 
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

HCURSOR EMUCollectDialog::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}



BOOL EMUCollectDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	//Call PC's init function...
	SetupGlobals();

	//build and fetch all of our little components.

	OpenOpcodeList = new CFileDialog( true, "EQ Logs (*.txt,*.bf)|*.txt; *.bf|Old Text Logs (*.txt)|*.txt|Build Files (*.bf)|*.bf", "", NULL, NULL, this );
	SaveOpcodeList = new CFileDialog( false, "txt", "*.txt", NULL, NULL, this );
	SavePacketFile = new CFileDialog( false, "pf", "*.pf", NULL, NULL, this );
	SavePacketText = new CFileDialog( false, "txt", "*.txt", NULL, NULL, this );

	//for whatever stupid reason, VS will not manage radio button
	//controls for me...
	//radio buttons:
	sItemsOnlyRadio = static_cast<CButton*>(GetDlgItem(IDC_ITEMSONLY));
	sAllPacketsRadio = static_cast<CButton*>(GetDlgItem(IDC_ALLPACKETS));
	sNoFilterRadio = static_cast<CButton*>(GetDlgItem(IDC_FilterNone));
	sIncludeFilterRadio = static_cast<CButton*>(GetDlgItem(IDC_FilterInclude));
	sExcludeFilterRadio = static_cast<CButton*>(GetDlgItem(IDC_FilterExclude));
	
	sAllPacketsRadio->SetCheck(BST_CHECKED);
	sNoFilterRadio->SetCheck(BST_CHECKED);

	CArray<CString, CString&> devlist;
	GetDeviceList(devlist);

	int s;
	s = devlist.GetSize();
	if(s == 0) {
		m_Device.AddString("ERROR LISTING DEVICES");
	} else {
		int r;
		for(r = 0; r < s; r++) {
			CString dname = devlist.GetAt(r);

			//skip the random empty entry I get some times.
			if(dname.GetLength() == 0)
				continue;
			m_Device.AddString(dname);
		}
	}
	m_Device.SetCurSel(0);

	
	if(OpcodeLoadFailed)
		m_StatsOpcodes.SetWindowText("Opcodes Loaded: NO");
	else
		m_StatsOpcodes.SetWindowText("Opcodes Loaded: YES");
	
	//privacy mode stuff:
	m_PrivacyName.SetLimitText(63);
	//ignore_world = true;		//GUI ignores world by default
	//m_IgnoreWorld.SetCheck(BST_CHECKED);
	
	//GUI starts with DB upload disabled
	//nodb = true;
	//start with items disabled unless they enable DB
	//noitems = true;
	//GUI logs all opcodes by default
	//interested_opcodes[0] = 0xFFFF;
	
	ReadPrefsFile();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//prototype
void CollectThread(void* tmp);

void EMUCollectDialog::OnBeginCollecting() 
{
	char tmpstr[255];

	/*
	//handle the text logging setting
	if(m_TextLogs.GetCheck() == BST_CHECKED) {
		m_TextFile.GetLine(0, tmpstr, 255);
		if(strlen(tmpstr) < 1) {
			//ERROR...
			return;
		}

		opcodefile = new char[strlen(tmpstr)+1];
		strcpy(opcodefile, tmpstr);
	} else {
		opcodefile = NULL;
	}
	
	//handle the packet file logging setting...
	if(m_BinLogs.GetCheck() == BST_CHECKED) {
		m_BinFile.GetLine(0, tmpstr, 255);
		if(strlen(tmpstr) < 1) {
			//ERROR...
			return;
		}
		
		FILE *tmp = fopen(tmpstr, "r");
		if(tmp != NULL) {
			fclose(tmp);
			CString info;
			info = "The packet file ";
			info += tmpstr;
			info += " allready exist, do you want to overwrite this file with your next collect?";
			ConfirmDialog conf(info);
			if(conf.DoModal() != IDOK)
				return;
		}

		packet_file = new PacketFileWriter(packet_file_flush);
		if(!packet_file->OpenFile(tmpstr)) {
			//ERROR...
			return;
		}
	} else {
		packet_file = NULL;
	}
	*/

	//if db is disabled, so is all the related stuff...
/*	if(nodb) {
		noitems = true;
		items_only = false;
	}
	//if we have no db or we ony want items, disable the junk
	if(nodb || items_only) {
		lognewzonetocfg = false;
		logzoneobjects = false;
		logdoors = false;
		logspawns = false;
		logzonepoints = false;
		loggrids = false;
	}
	*/
	
	//handle privacy mode stuff
	if(privacy_mode) {
		m_PrivacyName.GetLine(0, privacy_name, 64);
	}

	//pull out our sniffing IP
	memset(sniffhost, 0, sizeof(sniffhost));
	m_WatchIP.GetLine(0, sniffhost, 255);

/*	char errbuf[ERRBUF_SIZE];
	eqpsl.sniffIP = ResolveIP(sniffhost, errbuf);
	if (eqpsl.sniffIP == INADDR_NONE || eqpsl.sniffIP == 0) {
		//ERROR...
		return;
	}
*/
  
	int sel = m_Device.GetCurSel() + 1;

	if(!LookupPcapDevice(sel)) {
		//ERROR...
		return;
	}
	
/*	int argc = 2;
	const char *argv[3];
	//fake cmdline args to put into log
	argv[0] = "packetcollector.exe";
	argv[1] = "GUI MODE";
*/	SetupCollectEnvironment();
	
	
	/*
		Set up the plugins for collecting.
	*/
	ClearPacketHandlers();
	
	//this should only ever be done once, like loading a plugin would be.
	HandlerCallbacks cbs;
	GetHandlerCallbacks(&cbs);
	
	//add callback for mob info handling
	cbs.AddStreamCreateHandler(CollectMobCreateHandler);
	//add packet header callback too
	//SetupPacketHeaderHandler(&cbs, true);
	
	if(privacy_mode) {
		//setup the handler for privacy mode pfs
		SetupPrivacyPacketFileCollector(&cbs, privacy_name);
	} else {
		//add callback for packet handling from plugin (statically linked)
		SetupPacketFileCollector(&cbs);
	}
	
	
	if(!SetupLiveCollect()) {
		//ERROR...
		return;
	}

	//disable things which will not be changeable for the rest of the run
	m_WatchIP.EnableWindow(false);
	m_Device.EnableWindow(false);


	//set our status and disable everything...
	m_StatsRunning.SetWindowText("Running: YES");

	m_StartButton.EnableWindow(false);
	m_StopButton.EnableWindow(true);

	//start the collecting thread running...
	_beginthread(CollectThread, 0, this);
}

void EMUCollectDialog::OnStopCollecting() 
{
	DoLoop = false;
}

void EMUCollectDialog::CollectionComplete() {
	
	//set our status and re-enable our controls...
	m_StatsRunning.SetWindowText("Running: no");
	
	m_StartButton.EnableWindow(true);
	m_StopButton.EnableWindow(false);
}

void CollectThread(void* tmp) {
	mode = liveCollectMode;
	StartCollecting();
	ClearCollectEnvironment();

	EMUCollectDialog *it = (EMUCollectDialog *) tmp;
	it->CollectionComplete();
}
	
void EMUCollectDialog::OnDBUploadCheck() 
{
/*	bool mode;
	mode = (m_DBUpload.GetCheck() == BST_CHECKED);
	
	nodb = !mode;
	
	//these depend on being in all packets mode...
	bool mode2 = mode && (sAllPacketsRadio->GetCheck() == BST_CHECKED);
	m_CollectZones.EnableWindow(mode2);
	m_CollectSpawns.EnableWindow(mode2);
	m_CollectObjects.EnableWindow(mode2);
	m_CollectItems.EnableWindow(mode2);
	m_CollectGrids.EnableWindow(mode2);
	m_CollectDoors.EnableWindow(mode2);

	m_UploadLabel.EnableWindow(mode);
	m_UploadName.EnableWindow(mode);

	sItemsOnlyRadio->EnableWindow(mode);
	sAllPacketsRadio->EnableWindow(mode);

	//m_SC_group.EnableWindow(mode);
	*/
}

void EMUCollectDialog::ReadPrefsFile() {
	PrefsFileData p;
	
	memset(&p, 0, sizeof(p));
	
	FILE *pf = fopen("prefs.dat", "rb");
	if(pf == NULL)
		return;
	
	//just make sure we read something... the struct might have grown.
	if(fread(&p, 1, sizeof(p), pf) < 0) {
		//report error?
		fclose(pf);
		return;
	}

	fclose(pf);

	/*
		We call the event handlers for the controls so
		that we dot not have to duplicate their logic.
	*/

	m_Device.SetCurSel(p.pcap_device);
	m_WatchIP.SetWindowText(p.ipaddr);
	OnChangeWatchIp();

	m_DebugConsole.SetCheck(p.debug_console?BST_CHECKED:BST_UNCHECKED);
	OnDebugConsole();

	m_PrivacyMode.SetCheck(p.privacy_mode?BST_CHECKED:BST_UNCHECKED);
	OnPrivacyMode();

	m_IgnoreWorld.SetCheck(p.ignore_world?BST_CHECKED:BST_UNCHECKED);
	OnIgnoreWorld();

	m_ShowOpcodes.SetCheck(p.name_opcodes?BST_CHECKED:BST_UNCHECKED);
	OnShowOpcodes();

	m_OnMobInfo.SetCheck(p.mob_info?BST_CHECKED:BST_UNCHECKED);
	OnOnMobInfo();

	m_ForceFlush.SetCheck(p.force_flush?BST_CHECKED:BST_UNCHECKED);
	OnForceFlush();

	m_TextLogs.SetCheck(p.text_logging?BST_CHECKED:BST_UNCHECKED);
	OnTextlogs();
	m_TextFile.SetWindowText(p.text_file);

	m_BinLogs.SetCheck(p.binary_logging?BST_CHECKED:BST_UNCHECKED);
	OnBinlogs();
	m_BinFile.SetWindowText(p.binary_file);

	m_PrivacyName.SetWindowText(p.privacy_name);
}

void EMUCollectDialog::WritePrefsFile() {
	PrefsFileData p;
	
	FILE *pf = fopen("prefs.dat", "wb");
	if(pf == NULL) {
		//report error?
		return;
	}

	memset(&p, 0, sizeof(p));

	p.pcap_device = m_Device.GetCurSel();
	m_WatchIP.GetWindowText(p.ipaddr, 63);

	p.debug_console = (m_DebugConsole.GetCheck() == BST_CHECKED);

	p.privacy_mode = (m_PrivacyMode.GetCheck() == BST_CHECKED);

	p.ignore_world = (m_IgnoreWorld.GetCheck() == BST_CHECKED);

	p.name_opcodes = (m_ShowOpcodes.GetCheck() == BST_CHECKED);

	p.mob_info = (m_OnMobInfo.GetCheck() == BST_CHECKED);

	p.force_flush = (m_ForceFlush.GetCheck() == BST_CHECKED);

	p.text_logging = (m_TextLogs.GetCheck() == BST_CHECKED);
	m_TextFile.GetWindowText(p.text_file, 1023);

	p.binary_logging = (m_BinLogs.GetCheck() == BST_CHECKED);
	m_BinFile.GetWindowText(p.binary_file, 1023);

	m_PrivacyName.GetWindowText(p.privacy_name, 1023);


	
	if(fwrite(&p, 1, sizeof(p), pf) != sizeof(p)) {
		//report error?
		fclose(pf);
		return;
	}

	fclose(pf);
}


void EMUCollectDialog::OnShowOpcodes() 
{
//	showopcodenames = (m_ShowOpcodes.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnItemsonly() 
{
//	noitems = false;
//	items_only = true;
}

void EMUCollectDialog::OnAllpackets() 
{
//	noitems = false;
//	items_only = false;
}

void EMUCollectDialog::OnCollectItems() 
{
//	noitems = (m_CollectItems.GetCheck() != BST_CHECKED);
}

void EMUCollectDialog::OnCollectDoors() 
{
//	logdoors = (m_CollectDoors.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnCollectObjects() 
{
//	logzoneobjects = (m_CollectObjects.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnCollectZones() 
{
//	logzonepoints = (m_CollectZones.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnCollectSpawns() 
{
//	logspawns = (m_CollectSpawns.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnCollectGrids() 
{
//	loggrids = (m_CollectGrids.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnTextlogs() 
{
	dotextlogs = (m_TextLogs.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnBinlogs() 
{
	dobinlogs = (m_BinLogs.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnSelectTextLog() 
{
	// TODO: Add your control notification handler code here
}

void EMUCollectDialog::OnSelectBinLog() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnFilterNone() 
{ 
	filterMode = FILTER_NONE;
}

void EMUCollectDialog::OnFilterInclude() 
{
	filterMode = FILTER_INCLUDE;
}

void EMUCollectDialog::OnFilterExclude() 
{
	filterMode = FILTER_EXCLUDE;
}

void EMUCollectDialog::OnOpiAdd() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpiRemove() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpiLoad() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpiSave() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpeAdd() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpeRemove() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpeLoad() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnOpeSave() 
{
	// TODO: Add your control notification handler code here
	
}

void EMUCollectDialog::OnChangeWatchIp() 
{
	char tmpstr[255];

	m_WatchIP.GetLine(0, tmpstr, 255);

	if(strlen(tmpstr) > 6) {
		m_StartButton.EnableWindow(true);
	} else {
		m_StartButton.EnableWindow(false);
	}
}

void EMUCollectDialog::OnRawLogging() 
{
//	raw_logging = (m_RawLogging.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnDebugConsole() 
{
	if(m_DebugConsole.GetCheck() == BST_CHECKED) {
		AllocConsole();
		freopen ("CONOUT$", "w", stdout);
		freopen( "CONOUT$", "a", stderr);
	} else {
		FreeConsole();
	}
}

void EMUCollectDialog::OnAboutPrivacy() 
{
	PrivacyDialog bob;
	bob.DoModal();
}

void EMUCollectDialog::OnIgnoreWorld() 
{
//	ignore_world = (m_IgnoreWorld.GetCheck() == BST_CHECKED);
}

void EMUCollectDialog::OnPrivacyMode() 
{
	privacy_mode = (m_PrivacyMode.GetCheck() == BST_CHECKED);

	m_PrivacyLabel.EnableWindow(privacy_mode);
	m_PrivacyName.EnableWindow(privacy_mode);
}

void EMUCollectDialog::OnSaveSettings() 
{
	WritePrefsFile();
}

void EMUCollectDialog::OnMobInfo() 
{
	MobInfoDlg MobInfo;

	MobInfo.DoModal();
}

void EMUCollectDialog::OnOnMobInfo() 
{
	CollectMobInfo = (m_OnMobInfo.GetCheck() == BST_CHECKED);
	m_MobInfo.EnableWindow(CollectMobInfo);
}

void EMUCollectDialog::OnForceFlush() 
{
//	packet_file_flush = (m_ForceFlush.GetCheck() == BST_CHECKED);
}













