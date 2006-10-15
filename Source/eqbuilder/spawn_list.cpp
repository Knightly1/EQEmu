// spawn_list.cpp: implementation of the spawn_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "spawn_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

spawn_list::spawn_list()
{
}

spawn_list::~spawn_list() {
	vector<cspawn*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		delete *cur;
	}
}

void spawn_list::add( cspawn* spawn ) {
	list.push_back(spawn);
}

cspawn* spawn_list::get( int pos ) {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}

void spawn_list::remove(cspawn *spawn) {
	vector<cspawn*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		if(*cur == spawn) {
			list.erase(cur);
//			return;
		}
	}
}