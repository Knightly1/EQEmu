#if !defined(AFX_FACTIONSDIALOG_H__E786BEBE_CC48_4616_BFDB_8E6140F7FC7C__INCLUDED_)
#define AFX_FACTIONSDIALOG_H__E786BEBE_CC48_4616_BFDB_8E6140F7FC7C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FactionsDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// FactionsDialog dialog

class FactionsDialog : public CDialog
{
// Construction
public:
	FactionsDialog(vector<MatchStruct> *m_mapping, CWnd* pParent = NULL);   // standard constructor

	void UpdateLists(npc_list *npcs);

// Dialog Data
	//{{AFX_DATA(FactionsDialog)
	enum { IDD = IDD_FACTIONS };
	CTreeCtrl	m_SourceMappingList;
	CTreeCtrl	m_CurrentMappingList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(FactionsDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	
	vector<MatchStruct> *m_mapping;
	npc_list *m_npcs;

	// Generated message map functions
	//{{AFX_MSG(FactionsDialog)
	afx_msg void OnToggleFactions();
	afx_msg void OnAddMapping();
	afx_msg void OnEditMappings();
	afx_msg void OnDeleteMapping();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSelchangedSourceMappings(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FACTIONSDIALOG_H__E786BEBE_CC48_4616_BFDB_8E6140F7FC7C__INCLUDED_)
