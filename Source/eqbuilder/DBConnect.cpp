// DBConnect.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "DBConnect.h"
#include "EQBuilderDlg.h"
#include  <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DBConnect dialog


DBConnect::DBConnect( CString host, CString user, CString password, CString db, CWnd* pParent )
	: CDialog(DBConnect::IDD, pParent)
{
	//{{AFX_DATA_INIT(DBConnect)
	m_host = host;
	m_user = user;
	m_password = password;
	m_database = db;
	//}}AFX_DATA_INIT
}


void DBConnect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DBConnect)
	DDX_Text(pDX, IDC_DATABASE, m_database);
	DDX_Text(pDX, IDC_HOST, m_host);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDX_Text(pDX, IDC_USER, m_user);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DBConnect, CDialog)
	//{{AFX_MSG_MAP(DBConnect)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DBConnect message handlers

BOOL DBConnect::OnInitDialog() 
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void DBConnect::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

