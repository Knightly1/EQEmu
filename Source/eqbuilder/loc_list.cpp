// loc_list.cpp: implementation of the loc_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "loc_list.h"
#include "cloc.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

loc_list::loc_list()
{

}
loc_list::~loc_list()
{
	vector<cloc*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		delete *cur;
	}
}

loc_list::loc_list( loc_list *them ) {
	vector<cloc*>::iterator cur, end;
	cur = them->list.begin();
	end = them->list.end();
	int r;
	list.resize(them->getsize());
	for(r = 0; cur != end; cur++, r++) {
		list[r] = new cloc(*cur);
	}

}

void loc_list::add( cloc* loc ) {
	list.push_back(loc);
}

cloc* loc_list::get( int pos ) {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}




spawnpoint_list::spawnpoint_list()
{

}
spawnpoint_list::~spawnpoint_list()
{
	vector<cspawnpoint*>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		delete *cur;
	}
}

spawnpoint_list::spawnpoint_list( spawnpoint_list *them ) {
	vector<cspawnpoint*>::iterator cur, end;
	cur = them->list.begin();
	end = them->list.end();
	int r;
	list.resize(them->getsize());
	for(r = 0; cur != end; cur++, r++) {
		list[r] = new cspawnpoint(*cur);
	}

}

void spawnpoint_list::add( cspawnpoint* loc ) {
	list.push_back(loc);
}

cspawnpoint* spawnpoint_list::get( int pos ) {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}


void spawnpoint_list::get2Dradius(const cloc *center, float *radius_into) {
	float d2 = 0;


	vector<cspawnpoint *>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		const cspawnpoint *loc = *cur;
		
		float dx = center->x - loc->x;
		float dy = center->y - loc->y;
		float cd2 = dx*dx + dy*dy;
		
		if(cd2 > d2)
			d2 = cd2;
	}
	*radius_into = sqrt(d2);
}

void spawnpoint_list::getcenter(cloc *into) {

	vector<cspawnpoint *>::iterator cur, end;
	cur = list.begin();
	end = list.end();

	into->x = 0;
	into->y = 0;
	into->z = 0;
	into->heading = 0;
	float count = 0;
	
	const cspawnpoint* loc = NULL;
	int i = 0;
	for(; cur != end; cur++) {
		loc = *cur;
		
		into->x += loc->x;
		into->y += loc->y;
		into->z += loc->z;
		
		count++;
	}
	
	into->x /= count;
	into->y /= count;
	into->z /= count;
}

