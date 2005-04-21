// PrivacyDialog.cpp : implementation file
//

#include "stdafx.h"
#include "EMUCollect.h"
#include "PrivacyDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PrivacyDialog dialog


PrivacyDialog::PrivacyDialog(CWnd* pParent /*=NULL*/)
	: CDialog(PrivacyDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(PrivacyDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PrivacyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PrivacyDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PrivacyDialog, CDialog)
	//{{AFX_MSG_MAP(PrivacyDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PrivacyDialog message handlers
