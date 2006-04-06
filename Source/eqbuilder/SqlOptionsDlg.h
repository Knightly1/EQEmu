#if !defined(AFX_SQLOPTIONSDLG_H__FDA8A251_0D4D_46F0_AC23_29E02696B091__INCLUDED_)
#define AFX_SQLOPTIONSDLG_H__FDA8A251_0D4D_46F0_AC23_29E02696B091__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SqlOptionsDlg.h : header file
//
#include "BrowseForFolder.h"

/////////////////////////////////////////////////////////////////////////////
// SqlOptionsDlg dialog

class SqlOptionsDlg : public CDialog
{
// Construction
public:
	SqlOptionsDlg( int useopt, int zoneid, int npcid, int spawnid, int gridid, bool sqldelete, bool usedb, CString eqmaps_path, CString eqemumaps_path, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(SqlOptionsDlg)
	enum { IDD = IDD_SQLOPT };
	int		m_spawnid;
	int		m_npcid;
	int		m_gridid;
	int		m_zoneid;
	BOOL	m_sqldelete;
	int		m_useopt;
	BOOL	m_usedb;
	CString	m_EQMaps;
	CString	m_EQEmuMaps;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SqlOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CEdit* eNpcID;
	CEdit* eSpawnID; 
	CEdit* eGridID;
	CEdit* eZoneID;
	
	CEdit* eEQMPath;
	CEdit* eEQEMPath;;

	CBrowseForDirectory BrowseForDirectory;

	// Generated message map functions
	//{{AFX_MSG(SqlOptionsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDeleteCheck();
	afx_msg void OnUsezoneid();
	afx_msg void OnUsespecid();
	afx_msg void OnBrowseMaps();
	afx_msg void OnBrowseEmuMaps();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLOPTIONSDLG_H__FDA8A251_0D4D_46F0_AC23_29E02696B091__INCLUDED_)
