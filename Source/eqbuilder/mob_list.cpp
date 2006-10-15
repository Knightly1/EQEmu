// mob_list.cpp: implementation of the mob_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "mob_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

mob_list::mob_list()
{
}

mob_list::~mob_list()
{
	vector<cmob*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		delete *cur;
	}
}

mob_list::mob_list( mob_list* them ) 
{
	vector<cmob*>::iterator cur, end;
	cur = them->list.begin();
	end = them->list.end();
	int r;
	list.resize(them->getsize());
	for(r = 0; cur != end; cur++, r++) {
		list[r] = new cmob(*cur);
	}
}

void mob_list::add( cmob* mob ) {
	list.push_back(mob);
}

cmob* mob_list::get( int pos ) {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}

const cmob* mob_list::get( int pos ) const {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}

cmob* mob_list::GetMobByEntityId( int id, bool can_be_dead ) {
	vector<cmob*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {

		if ( (*cur)->entity_id == id ) {
			if(can_be_dead || !(*cur)->killed)
				return *cur;
		}

	}

	return NULL;


}

void mob_list::remove( cmob* mob ) {
	vector<cmob*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {

		if ( *cur == mob ) {
			list.erase(cur);
			return;
		}

	}


}
