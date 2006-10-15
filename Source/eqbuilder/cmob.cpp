// cmob.cpp: implementation of the cmob class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cmob.h"
#include "cmerchant.h"
#include "merchant_list.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cmob::cmob()
{
	this->entity_id = 0;
	this->db_spawnentry_id = 0;
	this->npc = NULL;
	this->hp = 0;
	this->owned = false;
	this->moved = false;
	this->roamed = false;
	this->type = 0;
	this->loc = NULL;
	this->speed = NULL;
	this->isnpc = false;
	this->typespawn = 0;
	this->killed = false;
	this->waypoints = NULL;
	this->gridid = 0;
	this->valid = true;
	db = false;
}

cmob::cmob( const cmob* mob )
{
	this->entity_id = mob->entity_id;
	this->db_spawnentry_id = mob->db_spawnentry_id;
	this->db = mob->db;
	this->npc = new cnpc( mob->npc );
	this->hp = mob->hp;
	this->owned = mob->owned;
	this->moved = mob->moved;
	this->roamed = mob->roamed;
	this->type = mob->type;
	this->loc = new cloc( mob->loc );
	if ( mob->speed != NULL ) {
		this->speed = new cloc( mob->speed );
	} else {
		this->speed = NULL;
	}
	this->isnpc = mob->isnpc;
	this->typespawn = mob->typespawn;
	this->killed = mob->killed;
	if ( mob->waypoints != NULL ) {
		this->waypoints = new waypoint_list( mob->waypoints );
	} else {
		this->waypoints = NULL;
	}
	this->gridid = mob->gridid;
	this->valid = mob->valid;
}

cmob::~cmob()
{
	delete npc;
	delete loc;
	delete speed;
	delete waypoints;
}
