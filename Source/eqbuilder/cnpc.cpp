// npc.cpp: implementation of the npc class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cnpc.h"
#include "cmerchant.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cnpc::cnpc()
{

	this->db = false;
	this->db_id = 0;
	this->nom = "";
	this->level = 0;
	this->gender = 0;
	this->size = 0;
	this->walkspeed = 0;
	this->runspeed = 0;
	this->race = 0;
	this->type = 0;
	this->classe = 0;
	this->texture = 0;
	this->helmtexture = 0;
	this->face = 0;
    this->luclin_hairstyle = 0;
	this->luclin_haircolor = 0;
	this->luclin_eyecolor = 0;
	this->luclin_eyecolor2 = 0;
	this->luclin_beard = 0;
	this->luclin_beardcolor = 0;
	this->merchant = NULL;
	this->faction_id = 0;
	this->loot_id = 0;
	this->spells_id = 0;

}

cnpc::cnpc( const cnpc* npc )
{

	this->db = npc->db;
	this->db_id = npc->db_id;
	this->nom = npc->nom;
	this->last_name = npc->last_name;
	this->level = npc->level;
	this->gender = npc->gender;
	this->size = npc->size;
	this->walkspeed = npc->walkspeed;
	this->runspeed = npc->runspeed;
	this->race = npc->race;
	this->type = npc->type;
	this->classe = npc->classe;
	this->texture = npc->texture;
	this->helmtexture = npc->helmtexture;
	this->face = npc->face;
    this->luclin_hairstyle = npc->luclin_hairstyle;
	this->luclin_haircolor = npc->luclin_haircolor;
	this->luclin_eyecolor = npc->luclin_eyecolor;
	this->luclin_eyecolor2 = npc->luclin_eyecolor2;
	this->luclin_beard = npc->luclin_beard;
	this->luclin_beardcolor = npc->luclin_beardcolor;

	if ( npc->merchant != NULL ) {
		this->merchant = new cmerchant( npc->merchant );
	} else {
		this->merchant = NULL;
	}

	this->faction_id = npc->faction_id;
	this->loot_id = npc->loot_id;
	this->spells_id = npc->spells_id;
}

cnpc::~cnpc()
{
	delete merchant;
}


bool cnpc::IsSameAs(const cnpc *npc2, bool compare_level) const {
	const cnpc *npc1 = this;

	if(npc1->db_id != 0 && npc1->db_id == npc2->db_id)
		return(true);
	
	return ( npc1->race == npc2->race )
		&& ( !compare_level || npc1->level == npc2->level )
		&& ( npc1->gender == npc2->gender )
		&& ( npc1->classe == npc2->classe )
		&& ( abs(npc1->size - npc2->size) < 0.1 )
		&& ( npc1->texture == npc2->texture )
		&& ( npc1->type == npc2->type )
//		&& ( npc1->helmtexture == npc2->helmtexture )		//disabled because older logs have unreliable helm entries
		&& ( abs(npc1->walkspeed - npc2->walkspeed) < 0.1 )
		&& ( abs(npc1->runspeed - npc2->runspeed) < 0.1 )
		&& ( npc1->nom.CompareNoCase(npc2->nom) == 0 )
		;
}









