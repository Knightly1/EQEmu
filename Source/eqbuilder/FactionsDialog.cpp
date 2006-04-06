// FactionsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "FactionsDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// FactionsDialog dialog


FactionsDialog::FactionsDialog(map<string, int> *mapping, CWnd* pParent /*=NULL*/)
	: CDialog(FactionsDialog::IDD, pParent),
	m_mapping(mapping)
{
	//{{AFX_DATA_INIT(FactionsDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_npcs = NULL;
}


void FactionsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(FactionsDialog)
	DDX_Control(pDX, IDC_SOURCE_MAPPINGS, m_SourceMappingList);
	DDX_Control(pDX, IDC_CURRENT_MAPPINGS, m_CurrentMappingList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(FactionsDialog, CDialog)
	//{{AFX_MSG_MAP(FactionsDialog)
	ON_BN_CLICKED(IDC_TOGGLE_FACTIONS, OnToggleFactions)
	ON_BN_CLICKED(IDC_ADD_MAPPING, OnAddMapping)
	ON_BN_CLICKED(IDC_EDIT_MAPPINGS, OnEditMappings)
	ON_BN_CLICKED(IDC_DELETE_MAPPING, OnDeleteMapping)
	ON_WM_SHOWWINDOW()
	ON_NOTIFY(TVN_SELCHANGED, IDC_SOURCE_MAPPINGS, OnSelchangedSourceMappings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// FactionsDialog message handlers

void FactionsDialog::OnToggleFactions() 
{
	// TODO: Add your control notification handler code here
	
}

void FactionsDialog::OnAddMapping() 
{
	// TODO: Add your control notification handler code here
	
}

void FactionsDialog::OnEditMappings() 
{
	// TODO: Add your control notification handler code here
	
}

void FactionsDialog::OnDeleteMapping() 
{
	// TODO: Add your control notification handler code here
	
}

void FactionsDialog::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	if(bShow) {
		if(m_npcs) {
		}

		lSpawnList->DeleteAllItems();
		vector<MatchStruct>::iterator cur, end;
		cur = m_mapping->begin();
		end = m_mapping->end();
		int r;
		for(r = 0; cur != end; cur++, r++) {
			CString p;
			p.Format("%s -> %d", cur->match, cur->id);
			HTREEITEM entry = m_SourceMappingList.InsertItem( p );
			m_SourceMappingList.SetItemData(entry, r);
		}
		lSpawnList->Expand( entry, TVE_EXPAND );
	}
	
}

void FactionsDialog::OnSelchangedSourceMappings(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}
