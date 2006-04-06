// npc_list.cpp: implementation of the npc_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "npc_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

npc_list::npc_list()
{
}

npc_list::~npc_list()
{
	vector<cnpc*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		delete *cur;
	}
}

void npc_list::add( cnpc* npc ) {
	list.push_back(npc);
}

cnpc* npc_list::get( int pos ) const {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}
