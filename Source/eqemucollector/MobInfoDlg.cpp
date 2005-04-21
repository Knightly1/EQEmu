// MobInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EMUCollect.h"
#include "MobInfoDlg.h"
#include "../common/classes.h"
#include "mobinfo_handler.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MobInfoDlg dialog


MobInfoDlg::MobInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(MobInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(MobInfoDlg)
	//}}AFX_DATA_INIT
}


void MobInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MobInfoDlg)
	DDX_Control(pDX, IDC_MOBLIST, m_MobList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(MobInfoDlg, CDialog)
	//{{AFX_MSG_MAP(MobInfoDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MobInfoDlg message handlers


void MobInfoDlg::SetMobList() {
	map<int32, CollectMobStruct>::iterator cur,end;
	cur = CollectMobList.begin();
	end = CollectMobList.end();
	
	m_MobList.ResetContent();

	for(; cur != end; cur++) {
		CString tmp;
		if(cur->second._class == MERCHANT) {
			tmp.Format("%s\tMerchant\t%d/%d\t%s", cur->second.name.c_str(), cur->second.wp_count, cur->second.move_count, cur->second.found_merchant?"YES":"NO");
		} else {
			tmp.Format("%s\t%s\t%d/%d\t", cur->second.name.c_str(), GetEQClassName(cur->second._class), cur->second.wp_count, cur->second.move_count);
		}
		m_MobList.AddString(tmp);
	}

	m_MobList.InsertString(0, "Name\tClass\tWaypoints\tInventory?");
}

BOOL MobInfoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int stops[4];
	stops[0] = 120;
	stops[1] = 200;
	stops[2] = 225;
	stops[2] = 250;

	m_MobList.SetTabStops(4, stops);

	SetMobList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
