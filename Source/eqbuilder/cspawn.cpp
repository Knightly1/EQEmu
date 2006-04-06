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
	this->id = 0;
	this->locs = new loc_list();
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
}

cspawn::cspawn( cspawn* spawn )
{
	this->db = spawn->db;
	this->is_camp = spawn->is_camp;
	this->falsespawn = spawn->falsespawn;
	if ( spawn->grid != NULL ) {
		this->grid = new cgrid( spawn->grid );
	} else {
		this->grid = NULL;
	}
	this->id = spawn->id;
	this->locs = new loc_list( spawn->locs );
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
}

cspawn::~cspawn()
{
	delete grid;
	delete locs;
	delete mobs;
}
