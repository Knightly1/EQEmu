#if !defined(AFX_DBCONNECT_H__A2B05C59_0B9B_4CF5_82BF_0C788EDFF9F9__INCLUDED_)
#define AFX_DBCONNECT_H__A2B05C59_0B9B_4CF5_82BF_0C788EDFF9F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DBConnect.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DBConnect dialog

class DBConnect : public CDialog
{
// Construction
public:
	DBConnect( CString host, CString user, CString password, CString db, CWnd* pParent = NULL );   // standard constructor

// Dialog Data
	//{{AFX_DATA(DBConnect)
	enum { IDD = IDD_DATABASE };
	CString	m_database;
	CString	m_host;
	CString	m_password;
	CString	m_user;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DBConnect)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

private:
	CString ebinipath;
	void LoadINI();
	void SaveINI();
	
	// Generated message map functions
	//{{AFX_MSG(DBConnect)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBCONNECT_H__A2B05C59_0B9B_4CF5_82BF_0C788EDFF9F9__INCLUDED_)
