#if !defined(AFX_MOBINFODLG_H__250B96CE_1ECD_4008_B116_9F18FC7CB25F__INCLUDED_)
#define AFX_MOBINFODLG_H__250B96CE_1ECD_4008_B116_9F18FC7CB25F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MobInfoDlg.h : header file
//

#include "pcvars.h"

/////////////////////////////////////////////////////////////////////////////
// MobInfoDlg dialog

class MobInfoDlg : public CDialog
{
// Construction
public:
	MobInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(MobInfoDlg)
	enum { IDD = IDD_MOBINFO };
	CListBox	m_MobList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MobInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetMobList();
protected:

	// Generated message map functions
	//{{AFX_MSG(MobInfoDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOBINFODLG_H__250B96CE_1ECD_4008_B116_9F18FC7CB25F__INCLUDED_)
