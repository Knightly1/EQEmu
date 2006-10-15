// cspawn.h: interface for the cspawn class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSPAWN_H__EC93C554_B470_41A5_9615_F971F85D0BA8__INCLUDED_)
#define AFX_CSPAWN_H__EC93C554_B470_41A5_9615_F971F85D0BA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "cloc.h"
#include "cgrid.h"
#include "mob_list.h"
#include "loc_list.h"


//a spawn group
class cspawn  
{
public:
	cspawn();
	cspawn( cspawn* spawn );
	virtual ~cspawn();

	bool db;
	int db_id;
	CString name;
	int spawngroupID;
	int nbmobs;
	int probability;
	mob_list* mobs;
	bool truespawn;
	bool falsespawn;
	cgrid* grid;
	int validmobs;
	
	bool roaming;
	bool is_camp;
	bool is_special;	//contains a special-class mob (non-playable class)
	
	cloc center;
	spawnpoint_list *locs;
	
	//used by zone viewer
	int r,g,b;	//spawn color

	bool ContainsNPC(const cnpc *npc, bool compare_level);
	void takeMobs(cspawn *from);

};

#endif // !defined(AFX_CSPAWN_H__EC93C554_B470_41A5_9615_F971F85D0BA8__INCLUDED_)
