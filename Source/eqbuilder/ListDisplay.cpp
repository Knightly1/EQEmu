// EQBuilderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include "globals.h"
#include "../common/types.h"
#include "../common/classes.h"
#include "ZoneViewer.h"
#include "BigZoneDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CEQBuilderDlg::CountMerchants(int &msets, int &mcount) {
	msets = 0;
	mcount = 0;

	int goal = listNPCs->getsize();
	for ( int i = 0; i < goal;i++ ) {
		cnpc* npc = listNPCs->get(i);

		if(npc->classe == MERCHANT)
			mcount++;
		if(npc->merchant != NULL)
			msets++;
	}
}

void CEQBuilderDlg::SetDataDisplay(int index) {
	switch(index) {
	case 0:
		showSpawnGroups();
		break;
	case 1:
		showMerchants();
		break;
	default:
		lSpawnList->DeleteAllItems();
		break;
	}
}

void CEQBuilderDlg::afficherSpawns() {
	pProgress->SetPos(0);

	lMobList->DeleteAllItems();

	int maxpos = listNPCs->getsize();

	for ( int i = 0; i<maxpos;i++ ) {
		cnpc* npc = listNPCs->get(i);

		CString line = npc->nom;
		line += " ";
		line += npc->last_name;
		line += " (";
		line += getClassName(npc->classe);
		line += "), level ";
		CString lvl;
		lvl.Format("%d", npc->level);
		line += lvl;

		HTREEITEM entry = lMobList->InsertItem( line );

		CString extra;
		extra.Format("Race %d, Size %.3f, Texture %d, Speed %.2f-%.2f", npc->race, npc->size, npc->texture, npc->walkspeed, npc->runspeed);
		HTREEITEM ph = lMobList->InsertItem( extra, entry );
		



		// progression
		pProgress->SetPos( ( i * 50 ) / maxpos );
	}
	lMobList->SortChildren(NULL);
}

void CEQBuilderDlg::showSpawnGroups() {
	lSpawnList->DeleteAllItems();

	if(!fixedSpawns || !gridSpawns)
		return;

	int maxpos = fixedSpawns->getsize() + gridSpawns->getsize();
	int curpos = 0;

	for ( int i = 0; i<fixedSpawns->getsize();i++ ) {
		cspawn* spawn = fixedSpawns->get(i);

		HTREEITEM entry = lSpawnList->InsertItem( "Fixed Spawn Group:" );
		HTREEITEM entries = lSpawnList->InsertItem( "Mobs:", entry );
		HTREEITEM locs = lSpawnList->InsertItem( "Locations:", entry );
		
		int j;
		int smd = spawn->mobs->getsize();
		for ( j=0; j<smd; j++ ) {
			lSpawnList->InsertItem( spawn->mobs->get(j)->npc->nom, entries );
		}
		
		CString sloc;
		smd = spawn->locs->getsize();
		for ( j=0; j<smd; j++ ) {
			cspawnpoint *loc = spawn->locs->get(j);
			sloc.Format( "Loc : (%f,%f,%f) %d chance", loc->x, loc->y, loc->z, spawn->probability );
			lSpawnList->InsertItem( sloc, locs );
		}

		lSpawnList->Expand( entry, TVE_EXPAND );
		lSpawnList->Expand( entries, TVE_EXPAND );
		lSpawnList->Expand( locs, TVE_EXPAND );

		// progression
		curpos ++;
		pProgress->SetPos( 50 + ( curpos * 50 ) / maxpos );
	}

	for ( int j = 0; j<gridSpawns->getsize();j++ ) {
		cspawn* spawn = gridSpawns->get(j);

		HTREEITEM entry = lSpawnList->InsertItem( "Roaming Spawn Group:" );
		HTREEITEM entries = lSpawnList->InsertItem( "Mobs:", entry );
		HTREEITEM locs = lSpawnList->InsertItem( "Locations:", entry );
		HTREEITEM path = lSpawnList->InsertItem( "Path:", entry );
		
		for ( int o=0; o<spawn->mobs->getsize(); o++ ) {
			cmob* mob = spawn->mobs->get(o);

			HTREEITEM wps = lSpawnList->InsertItem( mob->npc->nom, entries );
			CString lstr;
			lstr.Format( "Loc: x:%f y:%f z:%f h: %f", mob->loc->x, mob->loc->y, mob->loc->z, mob->loc->heading );
			lSpawnList->InsertItem( lstr, wps );

			if ( mob->waypoints != NULL ) {
				cwaypoint *lwp = mob->waypoints->get(0);
				for ( int k=0; k< mob->waypoints->getsize(); k++ ) {
					cwaypoint* wp = mob->waypoints->get(k);
					CString wpstr;
					wpstr.Format( "x:%f y:%f z:%f h: %f %s", wp->loc->x, wp->loc->y, wp->loc->z, wp->loc->heading, wp->valid?(wp->colinear?"*CO-LINEAR*":""):" **INVALID**" );
					lSpawnList->InsertItem( wpstr, wps );
					lwp = wp;
				}
			}
		}
		
		//we really should only have one point if they are a roamer
		CString sloc;
		int smd = spawn->locs->getsize();
		for ( int j=0; j<smd; j++ ) {
			cspawnpoint *loc = spawn->locs->get(j);
			sloc.Format( "Loc : (%f,%f,%f @%f) %d chance", loc->x, loc->y, loc->z, loc->heading, spawn->probability );
			lSpawnList->InsertItem( sloc, locs );
		}
		
		if(spawn->grid && spawn->grid->waypoints) {
			int wp = spawn->grid->waypoints->getsize();
			int r;
			for(r = 0; r  < wp; r++) {
				cwaypoint *pt = spawn->grid->waypoints->get(r);
				cloc *loc = pt->loc;
				sloc.Format( "Point : (%f,%f,%f)", loc->x, loc->y, loc->z );
				lSpawnList->InsertItem( sloc, path );
			}
		}
		
		lSpawnList->Expand( entry, TVE_EXPAND );
		lSpawnList->Expand( entries, TVE_EXPAND );
		lSpawnList->Expand( locs, TVE_EXPAND );

		// progression
		curpos ++;
		pProgress->SetPos( 50 + ( curpos * 50 ) / maxpos );
	}
}

void CEQBuilderDlg::showMerchants() {
	lSpawnList->DeleteAllItems();
	
	if(!listShops)
		return;

	int maxpos = listShops->getsize();
	int curpos = 0;

	map<uint16, uint32>::iterator cur,end;
	for ( int i = 0; i<listShops->getsize();i++ ) {
		cmerchant* m = listShops->get(i);
		
		CString n;
		n = "Merchant: ";
		if(m->owner != NULL) {
			n += m->owner->nom;
			if(!m->owner->last_name.IsEmpty()) {
				n += " " + m->owner->last_name;
			}
		} else {
			continue;	//no owner, skip this thing
		}
		HTREEITEM entry = lSpawnList->InsertItem( n );

		
		cur = m->items.begin();
		end = m->items.end();
		for(; cur != end; cur++) {
			CString in;
			in.Format("Item %d: #%d", cur->first, cur->second);
			lSpawnList->InsertItem( in, entry );
		}

	}
	pProgress->SetPos( 100 );
}

int CEQBuilderDlg::nbdoors() {
	if ( doors != NULL ) {
		return doors->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbteleports() {
	if ( teleports != NULL ) {
		return teleports->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbmobs() {
	int nbmobs = 0;
	for ( int i = 0; i<nblogs; i++ ) {
		if ( compiledlogs->get(i)->compiled ) {
			nbmobs = nbmobs + compiledlogs->get(i)->nbmobinit + compiledlogs->get(i)->nbmobadd;
		}
	}
	return nbmobs;
}

int CEQBuilderDlg::nbnpcs() {
	if ( listNPCs != NULL ) {
		return listNPCs->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbgrids() {
	if ( listGrids != NULL ) {
		return listGrids->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbgspawns() {
	if ( gridSpawns != NULL ) {
		return gridSpawns->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbfspawns() {
	if ( fixedSpawns != NULL ) {
		return fixedSpawns->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbmerchants() {
	if ( listShops != NULL ) {
		return listShops->getsize();
	} else {
		return 0;
	}
}

int CEQBuilderDlg::nbvalidspawns() {
	if ( ( fixedSpawns != NULL ) && ( gridSpawns != NULL ) ) {
		return fixedSpawns->getsize() + gridSpawns->getsize();
	} else {
		return 0;
	}
}
CString CEQBuilderDlg::getClassName(int id) {
	switch(id) {

	case WARRIOR:
		return("Warrior");
		break;
	case CLERIC:
		return("Cleric");
		break;
	case PALADIN:
		return("Paladin");
		break;
	case RANGER:
		return("Ranger");
		break;
	case SHADOWKNIGHT:
		return("Shadow Knight");
		break;
	case DRUID:
		return("Druid");
		break;
	case MONK:
		return("Monk");
		break;
	case BARD:
		return("Bard");
		break;
	case ROGUE:
		return("Rogue");
		break;
	case SHAMAN:
		return("Shaman");
		break;
	case NECROMANCER:
		return("Necromancer");
		break;
	case WIZARD:
		return("Wizard");
		break;
	case MAGICIAN:
		return("Magician");
		break;
	case ENCHANTER:
		return("Enchanter");
		break;
	case BEASTLORD:
		return("Beastlord");
		break;
	case BERSERKER:
		return("Berserker");
		break;
	case WARRIORGM:
		return("Warrior GM");
		break;
	case CLERICGM:
		return("Cleric GM");
		break;
	case PALADINGM:
		return("Paladin GM");
		break;
	case RANGERGM:
		return("Ranger GM");
		break;
	case SHADOWKNIGHTGM:
		return("Shadow Knight GM");
		break;
	case DRUIDGM:
		return("Druid GM");
		break;
	case MONKGM:
		return("Monk GM");
		break;
	case BARDGM:
		return("Bard GM");
		break;
	case ROGUEGM:
		return("Rogue GM");
		break;
	case SHAMANGM:
		return("Shaman GM");
		break;
	case NECROMANCERGM:
		return("Necromancer GM");
		break;
	case WIZARDGM:
		return("Wizard GM");
		break;
	case MAGICIANGM:
		return("Magician GM");
		break;
	case ENCHANTERGM:
		return("Enchanter GM");
		break;
	case BEASTLORDGM:
		return("Beastlord GM");
		break;
	case BERSERKERGM:
		return("Berserker GM");
		break;
	case BANKER:
		return("Banker");
		break;
	case MERCHANT:
		return("Merchant");
		break;
	case ADVENTURERECRUITER:
		return("Adv. Recruiter");
		break;
	case ADVENTUREMERCHANT:
		return("Adv. Merchant");
		break;
	case CORPSE_CLASS:
		return "Corpse Class";
	case TRIBUTE_MASTER:
		return("Tribute Master");
		break;
	case GUILD_TRIBUTE_MASTER:
		return "Guild Tribute Master";
	}
	return("Unknown");
}


