// EQBuilder.h : main header file for the EQBUILDER application
//

#if !defined(AFX_EQBUILDER_H__C3ED496A_B49D_4648_BA28_3D2DF4C73691__INCLUDED_)
#define AFX_EQBUILDER_H__C3ED496A_B49D_4648_BA28_3D2DF4C73691__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

#ifndef __AFXWIN_H__
//	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEQBuilderApp:
// See EQBuilder.cpp for the implementation of this class
//

class CEQBuilderApp : public CWinApp
{
public:
	CEQBuilderApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEQBuilderApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEQBuilderApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EQBUILDER_H__C3ED496A_B49D_4648_BA28_3D2DF4C73691__INCLUDED_)
