// SqlOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "SqlOptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SqlOptionsDlg dialog


SqlOptionsDlg::SqlOptionsDlg( int useopt, int zoneid, int npcid, int spawnid, int gridid, bool sqldelete, bool usedb, CString eqmaps_path, CString eqemumaps_path, CWnd* pParent /*=NULL*/)
	: CDialog(SqlOptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(SqlOptionsDlg)
	m_spawnid = spawnid;
	m_npcid = npcid;
	m_gridid = gridid;
	m_zoneid = zoneid;
	m_sqldelete = sqldelete?TRUE:FALSE;
	m_useopt = 0;
	m_usedb = usedb?TRUE:FALSE;
	m_EQMaps = eqmaps_path;
	m_EQEmuMaps = eqemumaps_path;
	//}}AFX_DATA_INIT
}


void SqlOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SqlOptionsDlg)
	DDX_Text(pDX, IDC_SPAWNID, m_spawnid);
	DDV_MinMaxInt(pDX, m_spawnid, 0, 400000);
	DDX_Text(pDX, IDC_NPCID, m_npcid);
	DDV_MinMaxInt(pDX, m_npcid, 0, 400000);
	DDX_Text(pDX, IDC_GRIDID, m_gridid);
	DDV_MinMaxInt(pDX, m_gridid, 0, 400000);
	DDX_Text(pDX, IDC_ZONEID, m_zoneid);
	DDV_MinMaxInt(pDX, m_zoneid, 0, 1000);
	DDX_Check(pDX, IDC_DELETE_CHECK, m_sqldelete);
	DDX_Radio(pDX, IDC_USEZONEID, m_useopt);
	DDX_Check(pDX, IDC_USEDBID, m_usedb);
	DDX_Text(pDX, IDC_EQMAPS, m_EQMaps);
	DDX_Text(pDX, IDC_EQEMUMAPS, m_EQEmuMaps);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SqlOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(SqlOptionsDlg)
	ON_BN_CLICKED(IDC_DELETE_CHECK, OnDeleteCheck)
	ON_BN_CLICKED(IDC_USEZONEID, OnUsezoneid)
	ON_BN_CLICKED(IDC_USESPECID, OnUsespecid)
	ON_BN_CLICKED(IDC_BrowseMaps, OnBrowseMaps)
	ON_BN_CLICKED(IDC_BrowseEmuMaps, OnBrowseEmuMaps)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SqlOptionsDlg message handlers

BOOL SqlOptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	eNpcID = static_cast<CEdit*>(GetDlgItem(IDC_NPCID));
	eSpawnID = static_cast<CEdit*>(GetDlgItem(IDC_SPAWNID)); 
	eGridID = static_cast<CEdit*>(GetDlgItem(IDC_GRIDID));
	eZoneID = static_cast<CEdit*>(GetDlgItem(IDC_ZONEID));
	eEQMPath = static_cast<CEdit*>(GetDlgItem(IDC_EQMAPS));
	eEQEMPath = static_cast<CEdit*>(GetDlgItem(IDC_EQEMUMAPS));

	switch( m_useopt ) {
		case 0: 
			CheckDlgButton(IDC_USEZONEID,1);
			eNpcID->EnableWindow( FALSE );
			eSpawnID->EnableWindow( FALSE );
			eGridID->EnableWindow( FALSE );
			eZoneID->EnableWindow( TRUE );
			break;
		case 1:
			CheckDlgButton(IDC_USESPECID,1);
			eNpcID->EnableWindow( TRUE );
			eSpawnID->EnableWindow( TRUE );
			eGridID->EnableWindow( TRUE );
			eZoneID->EnableWindow( FALSE );
			break;
		default:
			CheckDlgButton(IDC_USEZONEID,1);
			eNpcID->EnableWindow( FALSE );
			eSpawnID->EnableWindow( FALSE );
			eGridID->EnableWindow( FALSE );
			eZoneID->EnableWindow( TRUE );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void SqlOptionsDlg::OnUsespecid() 
{
	m_useopt = 1;

	eNpcID->EnableWindow();
	eSpawnID->EnableWindow(); 
	eGridID->EnableWindow();
	eZoneID->EnableWindow( false );	

	this->UpdateData( TRUE );

}

void SqlOptionsDlg::OnUsezoneid() 
{
	m_useopt = 0;

	eNpcID->EnableWindow( false );
	eSpawnID->EnableWindow( false ); 
	eGridID->EnableWindow( false );
	eZoneID->EnableWindow();

	this->UpdateData( TRUE );
}

void SqlOptionsDlg::OnDeleteCheck() 
{

}

void SqlOptionsDlg::OnBrowseMaps() {
	if(BrowseForDirectory.Browse("Select folder containing Sony's Text Maps...")) {
		m_EQMaps = BrowseForDirectory.GetFolderPath();
		eEQMPath->SetWindowText(m_EQMaps);
	}
}

void SqlOptionsDlg::OnBrowseEmuMaps() {
	if(BrowseForDirectory.Browse("Select folder containing EQEmu .map files...")) {
		m_EQEmuMaps = BrowseForDirectory.GetFolderPath();
		eEQEMPath->SetWindowText(m_EQEmuMaps);
	}
}
