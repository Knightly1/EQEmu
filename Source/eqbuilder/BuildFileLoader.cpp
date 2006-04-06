// 
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include <math.h>
#include  <io.h>
#include "globals.h"
#include "../common/types.h"
#include "../common/buildfile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CEQBuilderDlg::load_build_file()
{
	CFileException* pError=NULL; 

	CString path = chemin + "\\logs\\" + zonename + "\\" + compiledlogs->get(selectedlog)->name;
	
	const char *path_c = (const char *) path.GetBufferSetLength(path.GetLength());
	
	BuildFileReader reader;
	
	if(!reader.OpenFile(path_c)) {
		//TODO: report the damned error...
		return;
	}
	
	PF_SectionType t;
	uint16 packlen;
	unsigned char buf[65536];
	
	int packets = 0;
	
	while((packlen = 65535) 
	  && reader.ReadSection(t, packlen, buf)) {
		packets++;
		
		switch(t) {
		case PF_OP_InitMobSpawn:
			doMobInit((const PF_MobSpawn *) buf, false);
			break;
		case PF_OP_MobSpawn:
			doMobInit((const PF_MobSpawn *) buf, true);
			break;
		case PF_OP_MobMovement:
			doMovement((const PF_MobMovement *) buf);
			break;
		case PF_OP_MobLocation:
			doWaypoint((const PF_MobLocation *) buf);
			break;
		case PF_OP_Death:
			doKilled((const PF_Death *) buf);
			break;
		case PF_OP_DeleteSpawn:
			doDeleted((const PF_DeleteSpawn *) buf);
			break;
		case PF_OP_MerchantBegin:
			doMerchantBegin((const PF_MerchantBegin *) buf);
			break;
		case PF_OP_MerchantItem:
			doMerchantItem((const PF_MerchantItem *) buf);
			break;
		default:
			break;
		}
		
		if(packets > 100) {
			pProgress->SetPos( ++ currentlog->compilepos );
			packets = 0;
		}
	}
	
	reader.CloseFile();
}


void CEQBuilderDlg::doMobInit(const PF_MobSpawn *it, bool is_add) {

	if ( currentlog->mobinit == NULL ) {
		currentlog->mobinit = new mob_list();
	}

	if ( currentlog->mobadd == NULL ) {
		currentlog->mobadd = new mob_list();
	}

	//hack... if we spawn a mob who's ID is taken, assume the old mob with that ID was killed and we missed it some how...
	cmob* existing_mob = NULL;
	existing_mob = currentlog->mobinit->getmobbyid( it->spawn_id );
	if ( existing_mob == NULL && currentlog->mobadd != NULL) {
		existing_mob = currentlog->mobadd->getmobbyid( it->spawn_id );
	}
	if(existing_mob != NULL) {
		if(!existing_mob->killed)
			existing_mob->killed = true;
	}

//if(string(it->name).find("#Vigilum") == string::npos)
//	return;



	cmob* mob = new cmob();
	mob->npc = new cnpc();

	mob->id = it->spawn_id;
	mob->npc->type = it->bodytype;

	CString mobnom;
	mobnom = it->name;
	int numpos = mobnom.FindOneOf("0123456789");
	//could prolly get last name here if we wanted...
	mob->npc->nom = mobnom.Left(numpos);
//	mob->npc->nom = mobnom.Left( mobnom.GetLength() - 2 );
	
	mob->npc->level = it->level;
	mob->npc->gender = it->gender;
	mob->hp = 100;

	//hope this works.
	int ownedid = (int) it->pet_owner_id; 
	mob->owned = ( ownedid > 0 );
		
	mob->npc->size = it->size;
	mob->npc->walkspeed = it->walkspeed;
	mob->npc->runspeed = it->runspeed;

	mob->npc->race = it->race;
	

	mob->type = it->npc;
	mob->npc->classe = it->class_;

	mob->loc = new cloc();
	mob->loc->x = it->x;
	mob->loc->y = it->y;
	mob->loc->z = it->z;
	mob->loc->heading = it->heading;

	mob->speed = new cloc();
	mob->speed->x = it->delta_x;
	mob->speed->y = it->delta_y;
	mob->speed->z = it->delta_z;
	mob->speed->heading = it->delta_heading;

	mob->npc->texture = it->equip_chest2;
	if ( mob->npc->texture == -1 || mob->npc->texture == 255 ) {
		mob->npc->texture = 0;
	}

	mob->npc->helmtexture = it->helm;
	if ( mob->npc->helmtexture == -1 ) {
		mob->npc->helmtexture = 0;
	}
	
	mob->npc->luclin_haircolor = it->haircolor;
	mob->npc->luclin_beardcolor = it->beardcolor;
	mob->npc->luclin_beard = it->beard;
	mob->npc->luclin_eyecolor = it->eyecolor1;
	mob->npc->luclin_eyecolor2 = it->eyecolor2;
	mob->npc->luclin_hairstyle = it->hairstyle;
	mob->npc->face = it->face;

	mob->isnpc = ( mob->npc->type == 1 || mob->npc->type == 66 );

	if ( IsValidSpawn( mob ) ) {
		if(is_add) {
			currentlog->mobadd->add( mob );
			currentlog->nbmobadd++;
		} else {
			currentlog->mobinit->add( mob );
			currentlog->nbmobinit++;
		}
	} else {
		delete [] mob;
	}
}

void CEQBuilderDlg::doMovement(const PF_MobMovement *it)
{
	if ( currentlog->type != logPathing )
		return;
	if ( currentlog->mobinit == NULL && currentlog->mobadd == NULL ) {
		return;
	}

	cwaypoint* move = new cwaypoint();

	move->id = it->spawn_id;

	move->loc = new cloc();
	move->loc->x = it->x;
	move->loc->y = it->y;
	move->loc->z = it->z;
	move->loc->heading = it->heading;

	cmob* mob = NULL;

	mob = currentlog->mobinit->getmobbyid( move->id );
	if ( mob == NULL && currentlog->mobadd != NULL) {
		mob = currentlog->mobadd->getmobbyid( move->id );
	}

	if ( mob != NULL ) {

		mob->roamed = true;
		mob->moved = true;

		if ( mob->waypoints == NULL ) {
			mob->waypoints = new waypoint_list();
		}

		mob->waypoints->add( move );

	} else {
		delete move;
	}

}


void CEQBuilderDlg::doKilled(const PF_Death *it)
{

	cmob* mob = NULL;

	mob = currentlog->mobinit->getmobbyid( it->spawn_id );
	if ( mob == NULL ) {
		mob = currentlog->mobadd->getmobbyid( it->spawn_id );
	}

	if ( mob != NULL ) {
		mob->killed = true;
	}
}

void CEQBuilderDlg::doDeleted(const PF_DeleteSpawn *it)
{

	cmob* mob = NULL;

	mob = currentlog->mobinit->getmobbyid( it->spawn_id );
	if ( mob == NULL ) {
		mob = currentlog->mobadd->getmobbyid( it->spawn_id );
	}

	if ( mob != NULL ) {
		mob->killed = true;
	}
}

void CEQBuilderDlg::doWaypoint(const PF_MobLocation *it) {
	if ( currentlog->type != logPathing )
		return;

	cmob* mob = NULL;
	mob = currentlog->mobinit->getmobbyid( it->spawn_id );
	if ( mob == NULL ) {
		mob = currentlog->mobadd->getmobbyid( it->spawn_id );
	}
	if(mob == NULL)
		return;

	

	if(it->spawn_id == 0x0696) {
		int x = it->spawn_id;
		x++;
	}

	cwaypoint* wp = new cwaypoint();

	wp->id = it->spawn_id;

	wp->loc = new cloc();
	wp->loc->x = it->y;		//inverted x,y... not sure why exactly...
	wp->loc->y = it->x;
	wp->loc->z = it->z;// * 10;	//WTF is *10 all about???
	wp->loc->heading = it->heading;

	wp->pause = true;


	mob->roamed = true;
	mob->moved = true;

	if ( mob->waypoints == NULL ) {
		mob->waypoints = new waypoint_list();
	}

	if ( mob->waypoints->nbpauses() == 0 ) {
		if ( isMovingAtInit( mob ) == true ) {
			mob->loc->x = wp->loc->x;
			mob->loc->y = wp->loc->y;
			mob->loc->z = wp->loc->z;
			mob->loc->heading = wp->loc->heading;
		}
	}

	mob->waypoints->add( wp );


}

void CEQBuilderDlg::doMerchantBegin(const PF_MerchantBegin *it) {
	//make sure we are collecting merchant data.
	/*if ( ( currentlog->type == 0 ) || ( currentlog->type == 2 ) ) {
		parsenext();
		parseread();
		return;
	}*/

	cmerchant* m = new cmerchant();

	int mobid = it->spawn_id;

	m->rate = 1.05f;	//TODO: find this in emu's packets

	cmob* mob = NULL;
	
	if(currentlog->mobinit != NULL)
		mob = currentlog->mobinit->getmobbyid( mobid );
	if ( mob == NULL && currentlog->mobadd != NULL) {
		mob = currentlog->mobadd->getmobbyid( mobid );
	}

	if ( mob == NULL ) {
		delete m;
		return;
	}
	
	m->id = db->getNextMerchantID();

	mob->npc->merchant = m;
	m->owner = mob->npc;
	cur_merchant = m;
}

void CEQBuilderDlg::doMerchantItem(const PF_MerchantItem *it) {
	int slot_id = 0;
	int merchant_slot = 0;
	int item_id = 0;
	
	slot_id = it->slot_id;
	item_id = it->item_id;
	merchant_slot = it->merchant_slot;
	
	if(cur_merchant != NULL && item_id != 0) {
		cur_merchant->items[slot_id] = item_id;
	}
	
}











