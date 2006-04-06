// LoadingMap.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "LoadingMap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// LoadingMap dialog


LoadingMap::LoadingMap(CWnd* pParent /*=NULL*/)
	: CDialog(LoadingMap::IDD, pParent)
{
	Create(IDD_LOADING_MAP);
	//{{AFX_DATA_INIT(LoadingMap)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void LoadingMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LoadingMap)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(LoadingMap, CDialog)
	//{{AFX_MSG_MAP(LoadingMap)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// LoadingMap message handlers
