// cspawn.cpp: implementation of the cspawn class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cspawn.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cspawn::cspawn()
{
	db = false;
	this->falsespawn = false;
	this->grid = NULL;
	this->db_id = 0;
	this->locs = new spawnpoint_list();
	this->mobs = NULL;
	this->name = "";
	this->nbmobs = 0;
	this->probability = 50;
	this->roaming = false;
	this->spawngroupID = 0;
	this->truespawn = false;
	this->validmobs = 0;
	this->is_camp = false;
	this->center.x = -1;
	this->center.y = -1;
	this->center.z = -1;
	is_special = false;
}

cspawn::cspawn( cspawn* spawn )
{
	this->db = spawn->db;
	this->db_id = spawn->db_id;
	this->is_camp = spawn->is_camp;
	this->falsespawn = spawn->falsespawn;
	if ( spawn->grid != NULL ) {
		this->grid = new cgrid( spawn->grid );
	} else {
		this->grid = NULL;
	}
	this->locs = new spawnpoint_list( spawn->locs );
	if(spawn->mobs != NULL)
		this->mobs = new mob_list( spawn->mobs );
	else
		mobs = NULL;
	this->name = spawn->name;
	this->nbmobs = spawn->nbmobs;
	this->probability = spawn->probability;
	this->roaming = spawn->roaming;
	this->spawngroupID = spawn->spawngroupID;
	this->truespawn = spawn->truespawn;
	this->validmobs = spawn->validmobs;
	this->center = spawn->center;
	this->is_special = spawn->is_special;
}

cspawn::~cspawn()
{
	delete grid;
	delete locs;
	delete mobs;
}

bool cspawn::ContainsNPC(const cnpc *npc, bool compare_level) {
	int max = mobs->getsize();
	int r;
	for(r = 0; r < max; r++) {
		cmob *mob = mobs->get(r);
		if(npc->IsSameAs(mob->npc, compare_level)) {
			return(true);
		}
	}
	return(false);
}

void cspawn::takeMobs(cspawn *from) {
	int ms = from->mobs->getsize();
	int k;
	for(k = 0; k < ms; k++) {
		cmob *mob = from->mobs->get(k);
		if(ContainsNPC(mob->npc, true))
			delete mob;	//its allready in there, just kill it
		else
			mobs->add(mob);	//add it
	}
	from->mobs->clear();
}
