#if !defined(AFX_BIGZONEDLG_H__6F9602D3_B9D3_42E6_9AE0_ED39C784A58D__INCLUDED_)
#define AFX_BIGZONEDLG_H__6F9602D3_B9D3_42E6_9AE0_ED39C784A58D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BigZoneDlg.h : header file
//
#pragma warning(disable:4786)
#include "ZoneViewer.h"

class CEQBuilderDlg;
class TextMapReader;
class spawn_list;
class npc_list;

/////////////////////////////////////////////////////////////////////////////
// BigZoneDlg dialog

class BigZoneDlg : public CDialog
{
// Construction
public:
	BigZoneDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(BigZoneDlg)
	enum { IDD = IDD_BIGZONE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(BigZoneDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

public:
	void SetDialog(CEQBuilderDlg *dlg);
	void SetLists(spawn_list *gridSpawns, spawn_list *fixedSpawns, npc_list *npcs);
	void ClearLists();
	void SetMapReader(TextMapReader *tmf);
	void SetMinSpawnRadius(float err);

protected:
	
	CEQBuilderDlg *m_dialog;

	void RedoMobList();
	CTreeCtrl *lMobList;
	npc_list *listNPCs;
	spawn_list *gridSpawns;
	spawn_list *fixedSpawns;

	//zone view
	CZoneViewer vZoneView;

	bool m_shown;

	// Generated message map functions
	//{{AFX_MSG(BigZoneDlg)
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnResetView();
	afx_msg void OnReprocess();
	afx_msg void OnIncreaseRes();
	afx_msg void OnDecreaseRes();
	afx_msg void OnDrawPaths();
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClickMoblist(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangedMobTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnDrawHilite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BIGZONEDLG_H__6F9602D3_B9D3_42E6_9AE0_ED39C784A58D__INCLUDED_)
