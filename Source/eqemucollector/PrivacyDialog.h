#if !defined(AFX_PRIVACYDIALOG_H__7C5AC910_B959_4F6B_A5A4_B71FFB27D214__INCLUDED_)
#define AFX_PRIVACYDIALOG_H__7C5AC910_B959_4F6B_A5A4_B71FFB27D214__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrivacyDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PrivacyDialog dialog

class PrivacyDialog : public CDialog
{
// Construction
public:
	PrivacyDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PrivacyDialog)
	enum { IDD = IDD_PRIVACY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PrivacyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PrivacyDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRIVACYDIALOG_H__7C5AC910_B959_4F6B_A5A4_B71FFB27D214__INCLUDED_)
