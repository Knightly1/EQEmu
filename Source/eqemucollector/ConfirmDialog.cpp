// onfirmDialog.cpp : implementation file
//

#include "stdafx.h"
#include "EMUCollect.h"
#include "ConfirmDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ConfirmDialog dialog


ConfirmDialog::ConfirmDialog(CString text, CWnd* pParent /*=NULL*/)
	: CDialog(ConfirmDialog::IDD, pParent)
{
	
	txt = text;
	//{{AFX_DATA_INIT(ConfirmDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void ConfirmDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ConfirmDialog)
	DDX_Control(pDX, IDC_TEXT, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ConfirmDialog, CDialog)
	//{{AFX_MSG_MAP(ConfirmDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ConfirmDialog message handlers

BOOL ConfirmDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	m_Text.SetWindowText(txt);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
