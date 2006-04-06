// cmob.h: interface for the cmob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMOB_H__F5129F45_75B2_49D9_A258_E5CF3B2F21D4__INCLUDED_)
#define AFX_CMOB_H__F5129F45_75B2_49D9_A258_E5CF3B2F21D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cloc.h"
#include "cnpc.h"
#include "waypoint_list.h"

class cmob  
{
public:
	cmob();
	cmob( cmob* mob );
	virtual ~cmob();

	bool db;
	int id;
	cnpc* npc;
	int hp;
	bool owned;
	bool moved;
	bool roamed;
	int type;
	cloc* loc;
	cloc* speed;
	bool isnpc;
	int typespawn;
	bool killed;
	waypoint_list* waypoints;
	int gridid;
	bool valid;

};

#endif // !defined(AFX_CMOB_H__F5129F45_75B2_49D9_A258_E5CF3B2F21D4__INCLUDED_)
