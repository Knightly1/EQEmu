// LootDialog.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "LootDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// LootDialog dialog


LootDialog::LootDialog(map<string, int> *mapping, CWnd* pParent /*=NULL*/)
	: CDialog(LootDialog::IDD, pParent),
	m_mapping(mapping)
{
	//{{AFX_DATA_INIT(LootDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void LootDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LootDialog)
	DDX_Control(pDX, IDC_CURRENT_MAPPINGS, m_CurrentMappingList);
	DDX_Control(pDX, IDC_SOURCE_MAPPINGS, m_SourceMappingList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(LootDialog, CDialog)
	//{{AFX_MSG_MAP(LootDialog)
	ON_BN_CLICKED(IDC_TOGGLE_LOOT, OnToggleLoot)
	ON_BN_CLICKED(IDC_ADD_MAPPING, OnAddMapping)
	ON_BN_CLICKED(IDC_EDIT_MAPPING, OnEditMapping)
	ON_BN_CLICKED(IDC_DELETE_MAPPING, OnDeleteMapping)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// LootDialog message handlers


void LootDialog::UpdateLists(npc_list *npcs) {
	m_npcs = npcs;
}


void LootDialog::OnToggleLoot() 
{
	// TODO: Add your control notification handler code here
	
}

void LootDialog::OnAddMapping() 
{
	// TODO: Add your control notification handler code here
	
}

void LootDialog::OnEditMapping() 
{
	// TODO: Add your control notification handler code here
	
}

void LootDialog::OnDeleteMapping() 
{
	// TODO: Add your control notification handler code here
	
}
