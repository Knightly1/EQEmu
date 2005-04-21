// EMUCollect.h : main header file for the EMUCOLLECT application
//

#if !defined(AFX_EMUCOLLECT_H__8965E4AA_301C_4C7C_983C_02093A82368F__INCLUDED_)
#define AFX_EMUCOLLECT_H__8965E4AA_301C_4C7C_983C_02093A82368F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEMUCollectApp:
// See EMUCollect.cpp for the implementation of this class
//

class CEMUCollectApp : public CWinApp
{
public:
	CEMUCollectApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEMUCollectApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEMUCollectApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMUCOLLECT_H__8965E4AA_301C_4C7C_983C_02093A82368F__INCLUDED_)
