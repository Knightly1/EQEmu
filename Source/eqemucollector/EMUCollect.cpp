// EMUCollect.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "EMUCollect.h"
#include "EMUCollectDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEMUCollectApp

BEGIN_MESSAGE_MAP(CEMUCollectApp, CWinApp)
	//{{AFX_MSG_MAP(CEMUCollectApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEMUCollectApp construction

CEMUCollectApp::CEMUCollectApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CEMUCollectApp object

CEMUCollectApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CEMUCollectApp initialization

BOOL CEMUCollectApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	EMUCollectDialog dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
