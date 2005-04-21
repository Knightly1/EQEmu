#if !defined(AFX_ONFIRMDIALOG_H__76A14E76_26D5_48B5_A77A_41449A692091__INCLUDED_)
#define AFX_ONFIRMDIALOG_H__76A14E76_26D5_48B5_A77A_41449A692091__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfirmDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ConfirmDialog dialog

class ConfirmDialog : public CDialog
{
// Construction
public:
	ConfirmDialog(CString text, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(ConfirmDialog)
	enum { IDD = IDD_DIALOG1 };
	CStatic	m_Text;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ConfirmDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString txt;
	// Generated message map functions
	//{{AFX_MSG(ConfirmDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ONFIRMDIALOG_H__76A14E76_26D5_48B5_A77A_41449A692091__INCLUDED_)
