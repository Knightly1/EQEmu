// BigZoneDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "BigZoneDlg.h"
#include "EQBuilderDlg.h"
#include "spawn_list.h"
#include "npc_list.h"
#include "../zone/map.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// BigZoneDlg dialog


BigZoneDlg::BigZoneDlg(CWnd* pParent /*=NULL*/)
	: CDialog(BigZoneDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(BigZoneDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_dialog = NULL;
	m_shown = false;

	listNPCs = NULL;
	lMobList = NULL;
	gridSpawns = NULL;
	fixedSpawns = NULL;
}


void BigZoneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(BigZoneDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_ZONEVIEW, vZoneView);
//	m_shown = true;
//	
}


BEGIN_MESSAGE_MAP(BigZoneDlg, CDialog)
	//{{AFX_MSG_MAP(BigZoneDlg)
	ON_BN_CLICKED(IDC_ZoomIn, OnZoomIn)
	ON_BN_CLICKED(IDC_ZoomOut, OnZoomOut)
	ON_BN_CLICKED(IDC_RESET_VIEW, OnResetView)
	ON_BN_CLICKED(IDC_REPROCESS, OnReprocess)
	ON_BN_CLICKED(IDC_INCREASE_RES, OnIncreaseRes)
	ON_BN_CLICKED(IDC_DECREASE_RES, OnDecreaseRes)
	ON_BN_CLICKED(IDC_DRAW_PATHS, OnDrawPaths)
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	ON_NOTIFY(NM_CLICK, IDC_MOB_TREE, OnClickMoblist)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MOB_TREE, OnSelchangedMobTree)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_DRAW_HILITE, OnDrawHilite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void BigZoneDlg::SetDialog(CEQBuilderDlg *dlg) {
	m_dialog = dlg;
}

void BigZoneDlg::ClearLists() {
	vZoneView.ClearSpawnLists();
	gridSpawns = NULL;
	fixedSpawns = NULL;
	listNPCs = NULL;
}

void BigZoneDlg::SetLists(spawn_list *gridSpawns2, spawn_list *fixedSpawns2, npc_list *npcs) {
	vZoneView.ClearSpawnLists();
	vZoneView.AddSpawnList(gridSpawns2);
	vZoneView.AddSpawnList(fixedSpawns2);
	gridSpawns = gridSpawns2;
	fixedSpawns = fixedSpawns2;
	listNPCs = npcs;
	RedoMobList();
}

void BigZoneDlg::SetMapReader(TextMapReader *tmf) {
	vZoneView.SetMapReader(tmf);
//	if(m_shown)
//		vZoneView.RedrawWindow();
}

void BigZoneDlg::SetMinSpawnRadius(float err) {
	vZoneView.SetMinSpawnRadius(err);
}

/////////////////////////////////////////////////////////////////////////////
// BigZoneDlg message handlers

void BigZoneDlg::OnZoomIn() 
{
	vZoneView.ZoomIn();
}

void BigZoneDlg::OnZoomOut() 
{
	vZoneView.ZoomOut();
}

void BigZoneDlg::OnResetView() 
{
	vZoneView.ResetView();
}

void BigZoneDlg::OnReprocess() 
{
	if(m_dialog != NULL)
		m_dialog->Reprocess();
	vZoneView.RedrawWindow();
}

void BigZoneDlg::OnIncreaseRes() 
{
	vZoneView.IncreaseResolution();
}

void BigZoneDlg::OnDecreaseRes() 
{
	vZoneView.DecreaseResolution();
}

void BigZoneDlg::OnDrawPaths() 
{
	vZoneView.ToggleDrawPaths();
	vZoneView.RedrawWindow();
}

void BigZoneDlg::OnClose() 
{
	CDialog::OnClose();

	m_shown = false;
}

void BigZoneDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	m_shown = true;
	RedoMobList();
}

enum {
	npcHighBits = 0x80000000,
	spawnFixedHighBits = 0x40000000,
	spawnGridHighBits = 0xC0000000,
	entryHighBitMask = 0xC0000000,
	entryLowBitMask = 0x3FFFFFFF
};

void BigZoneDlg::RedoMobList() {
	if(!m_shown || lMobList == NULL || listNPCs == NULL || gridSpawns == NULL)
		return;
	
	lMobList->DeleteAllItems();
	
	map<string, HTREEITEM> rel;
	map<string, HTREEITEM>::iterator res, cur, end;
	map<string, int> fixed_spawncount;
	map<string, int> grid_spawncount;
	
	TVINSERTSTRUCT ins;
	char buffer[256];

	int r, max;
	max = listNPCs->getsize();
	for(r = 0; r < max; r++) {
		cnpc *npc = listNPCs->get(r);
		
		string n = (const char *) npc->nom;
		res = rel.find(n);
		if(res != rel.end())
			continue;
		
		CString line;
		line.Format("%s", npc->nom);
		strcpy(buffer, (const char *) line);
		
		ins.hParent = NULL;
		ins.hInsertAfter = TVI_SORT /*TVI_LAST*/;
		ins.item.mask = TVIF_TEXT | TVIF_PARAM;
		ins.item.pszText = buffer;
		ins.item.lParam = npcHighBits | r;
		HTREEITEM npcentry = lMobList->InsertItem( &ins );
		
		rel[n] = npcentry;
		fixed_spawncount[n] = 0;
		grid_spawncount[n] = 0;
	}

	max = gridSpawns->getsize();
	for(r = 0; r < max; r++) {
		cspawn *spawn = gridSpawns->get(r);

		int mmax = spawn->mobs->getsize();
//		if(mmax < 2)
//			continue;
		int k;
		for(k = 0; k < mmax; k++) {
			cmob *mob = spawn->mobs->get(k);

			string n = (const char *) mob->npc->nom;
			res = rel.find(n);
			if(res == rel.end())
				continue;
			
			grid_spawncount[n]++;

			HTREEITEM entry = res->second;

			CString line;
			line.Format("Spawn at (%.3f,%.3f,%.3f @%d)", mob->loc->x, mob->loc->y, mob->loc->z, mob->loc->heading);
			strcpy(buffer, (const char *) line);
			
			ins.hParent = entry;
			ins.hInsertAfter = TVI_LAST;
			ins.item.mask = TVIF_TEXT | TVIF_PARAM;
			ins.item.pszText = buffer;
			ins.item.lParam = spawnGridHighBits | r;
			HTREEITEM locentry = lMobList->InsertItem( &ins );
			ins.hParent = locentry;
			
			if(spawn->grid != NULL && spawn->grid->waypoints != NULL) {
				int jmax;
				jmax = spawn->grid->waypoints->getsize();
				int j;
				cwaypoint *wp1, *wp2;
				wp1 = NULL;
				wp2 = NULL;	
				VERTEX start, end;
				float len, dot;
				for(j = 0; j < jmax; j++) {
					cwaypoint *wp = spawn->grid->waypoints->get(j);

					if(wp2) {
						//last walking vector
						start.x = wp1->loc->x - wp2->loc->x;
						start.y = wp1->loc->y - wp2->loc->y;
						start.z = 0;
						//current walking vector
						end.x = wp->loc->x - wp1->loc->x;
						end.y = wp->loc->y - wp1->loc->y;
						end.z = 0;
						//normalize both vectors
						len = sqrt(end.x*end.x + end.y*end.y);
						if(len < 0.000005f) {
							//close enough to 0
							//special value
							dot = -2;
						} else {
							end.x /= len;
							end.y /= len;
							len = sqrt(start.x*start.x + start.y*start.y);
							if(len < 0.000005f) {
								//close enough to 0
								//special value
								dot = -2;
							} else {
								start.x /= len;	//this len is used below!
								start.y /= len;

								//dot product of the two vectors:
								dot = start.x*end.x + start.y*end.y;
							}
						}
					} else
						dot = -1;
					
					CString line;
					line.Format("(%.3f,%.3f,%.3f @%.2f) %.3f", wp->loc->x, wp->loc->y, wp->loc->z, wp->loc->heading, dot);
					strcpy(buffer, (const char *) line);
					ins.item.pszText = buffer;
					lMobList->InsertItem( &ins );
					wp2 = wp1;
					wp1 = wp;
				}
			}
		}

	}
	//lazy duplication
	max = fixedSpawns->getsize();
	for(r = 0; r < max; r++) {
		cspawn *spawn = fixedSpawns->get(r);

		int mmax = spawn->mobs->getsize();
//		if(mmax < 2)
//			continue;
		int k;
		for(k = 0; k < mmax; k++) {
			cmob *mob = spawn->mobs->get(k);

			string n = (const char *) mob->npc->nom;
			res = rel.find(n);
			if(res == rel.end())
				continue;

			fixed_spawncount[n]++;

			HTREEITEM entry = res->second;

			CString line;
			line.Format("Spawn at (%.3f,%.3f,%.3f @%d)", mob->loc->x, mob->loc->y, mob->loc->z, mob->loc->heading);
			strcpy(buffer, (const char *) line);
			
			ins.hParent = entry;
			ins.hInsertAfter = TVI_LAST;
			ins.item.mask = TVIF_TEXT | TVIF_PARAM;
			ins.item.pszText = buffer;
			ins.item.lParam = spawnFixedHighBits | r;
			HTREEITEM locentry = lMobList->InsertItem( &ins );
		}

	}

	cur = rel.begin();
	end = rel.end();
	for(; cur != end; cur++) {
		HTREEITEM entry = cur->second;

		CString line;
		line.Format("Count: %d fixed, %d grid", fixed_spawncount[cur->first], grid_spawncount[cur->first]);
		strcpy(buffer, (const char *) line);
		
		ins.hParent = entry;
		ins.hInsertAfter = TVI_FIRST;
		ins.item.mask = TVIF_TEXT | TVIF_PARAM;
		ins.item.pszText = buffer;
		ins.item.lParam = 0;
		HTREEITEM locentry = lMobList->InsertItem( &ins );
	}

}

BOOL BigZoneDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	lMobList = static_cast<CTreeCtrl*>(GetDlgItem(IDC_MOB_TREE));
	RedoMobList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void BigZoneDlg::OnClickMoblist(NMHDR* pNMHDR, LRESULT* pResult) 
{
}

void BigZoneDlg::OnSelchangedMobTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	unsigned long high = pNMTreeView->itemNew.lParam & entryHighBitMask;
	unsigned long low = pNMTreeView->itemNew.lParam & entryLowBitMask;
	switch(high) {
	case npcHighBits:
		vZoneView.HiliteNPC(listNPCs->get(low));
		break;
	case spawnFixedHighBits:
		vZoneView.HiliteSpawn(fixedSpawns->get(low));
		break;
	case spawnGridHighBits:
		vZoneView.HiliteSpawn(gridSpawns->get(low));
		break;
	}

	vZoneView.RedrawWindow();

	*pResult = 0;
}

void BigZoneDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	m_shown = false;
	
}

void BigZoneDlg::OnDrawHilite() 
{
	vZoneView.ToggleDrawHilite();
	vZoneView.RedrawWindow();
}






