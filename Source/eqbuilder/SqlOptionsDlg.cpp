// SqlOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "SqlOptionsDlg.h"
#include "IDGenerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SqlOptionsDlg dialog


SqlOptionsDlg::SqlOptionsDlg(/* int useopt, int zoneid, int npcid, int spawnid, int gridid, 
		 bool sqldelete, bool usedb, */CString eqmaps_path, CString eqemumaps_path,
		IDGenSet *ids, CWnd* pParent /*=NULL*/)
	: CDialog(SqlOptionsDlg::IDD, pParent)
{
//	m_sqldelete = sqldelete?TRUE:FALSE;
	//{{AFX_DATA_INIT(SqlOptionsDlg)
	m_EQMaps = eqmaps_path;
	m_EQEmuMaps = eqemumaps_path;
	//}}AFX_DATA_INIT
	m_ids = ids;
}

void SqlOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_DELETE_CHECK, m_sqldelete);
	DDX_Text(pDX, IDC_EQMAPS, m_EQMaps);
	DDX_Text(pDX, IDC_EQEMUMAPS, m_EQEmuMaps);
	//{{AFX_DATA_MAP(SqlOptionsDlg)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SqlOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(SqlOptionsDlg)
	ON_BN_CLICKED(IDC_DELETE_CHECK, OnDeleteCheck)
	ON_BN_CLICKED(IDC_USEZONEID, OnUsezoneid)
	ON_BN_CLICKED(IDC_USESPECID, OnUsespecid)
	ON_BN_CLICKED(IDC_BrowseMaps, OnBrowseMaps)
	ON_BN_CLICKED(IDC_BrowseEmuMaps, OnBrowseEmuMaps)

	ON_BN_CLICKED(IDC_NPCID_DB, OnNpcidDb)
	ON_BN_CLICKED(IDC_NPCID_FIXED, OnNpcidFixed)
	ON_EN_CHANGE(IDC_NPCID, OnChangeNpcid)
	ON_BN_CLICKED(IDC_NPCID_FLOAT, OnNpcidFloat)
	ON_EN_CHANGE(IDC_NPCID_FVAL, OnChangeNpcidFval)
	ON_BN_CLICKED(IDC_NPCID_ZID, OnNpcidZid)
	ON_EN_CHANGE(IDC_NPCID_ZVAL, OnChangeNpcidZval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SqlOptionsDlg message handlers

BOOL SqlOptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
/*	eNpcID = static_cast<CEdit*>(GetDlgItem(IDC_NPCID));
	eSpawnID = static_cast<CEdit*>(GetDlgItem(IDC_SPAWNID)); 
	eGridID = static_cast<CEdit*>(GetDlgItem(IDC_GRIDID));
	eZoneID = static_cast<CEdit*>(GetDlgItem(IDC_ZONEID));
*/	eEQMPath = static_cast<CEdit*>(GetDlgItem(IDC_EQMAPS));
	eEQEMPath = static_cast<CEdit*>(GetDlgItem(IDC_EQEMUMAPS));

/*	switch( m_useopt ) {
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
	}*/

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

void SqlOptionsDlg::OnNpcidDb() 
{
//	Npcids->SetToDB();
}

void SqlOptionsDlg::OnNpcidFixed() 
{
//	Npcids->SetToFixed();
}

void SqlOptionsDlg::OnChangeNpcid() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	
}

void SqlOptionsDlg::OnNpcidFloat() 
{
//	Npcids->SetToFloating();
}

void SqlOptionsDlg::OnChangeNpcidFval() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	
}

void SqlOptionsDlg::OnNpcidZid() 
{
//	Npcids->SetToZone();
}

void SqlOptionsDlg::OnChangeNpcidZval() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	
}
