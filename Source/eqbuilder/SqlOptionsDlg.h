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

class IDGenSet;

class SqlOptionsDlg : public CDialog
{
// Construction
public:
	SqlOptionsDlg( /*int useopt, int zoneid, int npcid, int spawnid, 
		int gridid, bool sqldelete, bool usedb, */ CString eqmaps_path, 
		CString eqemumaps_path,
		IDGenSet *ids,
		CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_SQLOPT };
	BOOL	m_sqldelete;
	int		m_useopt;
	BOOL	m_usedb;
	CString	m_EQMaps;
	CString	m_EQEmuMaps;
	//{{AFX_DATA(SqlOptionsDlg)
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SqlOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	//pointers to our parent's ID gens
	IDGenSet *m_ids;
	
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
	afx_msg void OnNpcidDb();
	afx_msg void OnNpcidFixed();
	afx_msg void OnChangeNpcid();
	afx_msg void OnNpcidFloat();
	afx_msg void OnChangeNpcidFval();
	afx_msg void OnNpcidZid();
	afx_msg void OnChangeNpcidZval();
	
	afx_msg void OnSpawngroupidDb();
	afx_msg void OnSpawngroupidFixed();
	afx_msg void OnChangeSpawngroupid();
	afx_msg void OnSpawngroupidFloat();
	afx_msg void OnChangeSpawngroupidFval();
	afx_msg void OnSpawngroupidZid();
	afx_msg void OnChangeSpawngroupidZval();
	
	afx_msg void OnGrididDb();
	afx_msg void OnGrididFixed();
	afx_msg void OnChangeGridid();
	afx_msg void OnGrididFloat();
	afx_msg void OnChangeGrididFval();
	afx_msg void OnGrididZid();
	afx_msg void OnChangeGrididZval();

	afx_msg void OnMerchantidDb();
	afx_msg void OnMerchantidFixed();
	afx_msg void OnChangeMerchantid();
	afx_msg void OnMerchantidFloat();
	afx_msg void OnChangeMerchantidFval();
	afx_msg void OnMerchantidZid();
	afx_msg void OnChangeMerchantidZval();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLOPTIONSDLG_H__FDA8A251_0D4D_46F0_AC23_29E02696B091__INCLUDED_)
