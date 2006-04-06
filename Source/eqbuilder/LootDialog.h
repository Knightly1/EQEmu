#if !defined(AFX_LOOTDIALOG_H__33D4FF02_65B1_4B9C_8C35_628B8AB3DF80__INCLUDED_)
#define AFX_LOOTDIALOG_H__33D4FF02_65B1_4B9C_8C35_628B8AB3DF80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LootDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// LootDialog dialog

class LootDialog : public CDialog
{
// Construction
public:
	LootDialog(vector<MatchStruct> *mapping, CWnd* pParent = NULL);   // standard constructor
	
	void UpdateLists(npc_list *npcs);

// Dialog Data
	//{{AFX_DATA(LootDialog)
	enum { IDD = IDD_LOOT };
	CTreeCtrl	m_CurrentMappingList;
	CTreeCtrl	m_SourceMappingList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(LootDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	vector<MatchStruct> *m_mapping;
	npc_list *m_npcs;

	// Generated message map functions
	//{{AFX_MSG(LootDialog)
	afx_msg void OnToggleLoot();
	afx_msg void OnAddMapping();
	afx_msg void OnEditMapping();
	afx_msg void OnDeleteMapping();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOOTDIALOG_H__33D4FF02_65B1_4B9C_8C35_628B8AB3DF80__INCLUDED_)
