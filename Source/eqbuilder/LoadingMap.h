#if !defined(AFX_LOADINGMAP_H__B6C93ECF_EDD1_4EDE_A62A_7E14DD00C201__INCLUDED_)
#define AFX_LOADINGMAP_H__B6C93ECF_EDD1_4EDE_A62A_7E14DD00C201__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoadingMap.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// LoadingMap dialog

class LoadingMap : public CDialog
{
// Construction
public:
	LoadingMap(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(LoadingMap)
	enum { IDD = IDD_LOADING_MAP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(LoadingMap)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(LoadingMap)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOADINGMAP_H__B6C93ECF_EDD1_4EDE_A62A_7E14DD00C201__INCLUDED_)
