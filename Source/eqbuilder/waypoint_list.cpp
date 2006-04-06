// waypoint_list.cpp: implementation of the waypoint_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "waypoint_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

waypoint_list::waypoint_list()
{
}

waypoint_list::waypoint_list( waypoint_list *them )
{
	vector<cwaypoint *>::iterator cur, end;
	cur = them->list.begin();
	end = them->list.end();
	int r;
	list.resize(them->getsize());
	for(r = 0; cur != end; cur++, r++) {
		list[r] = new cwaypoint(*cur);
	}
}

waypoint_list::~waypoint_list()
{
	vector<cwaypoint *>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	for(; cur != end; cur++) {
		delete *cur;
	}
}

void waypoint_list::add( cwaypoint* waypoint ) {
	list.push_back(waypoint);
}

cwaypoint* waypoint_list::get( int pos ) {
	if(pos >= getsize() || pos < 0)
		return(NULL);
	return(list[pos]);
}

int waypoint_list::nbpauses() {

	vector<cwaypoint *>::iterator cur, end;
	cur = list.begin();
	end = list.end();
	int i = 0;
	for(; cur != end; cur++) {
		if((*cur)->pause)
			i++;
	}

	return i;

}

/*
This method was added to address an issue where for some reason, the first waypoint we
got for a mob was very far away from the actual path that the mob was walking.
the goal of this fix is to watch for the case where the first two points are far from
eachother, and drop the first point.
*/
void waypoint_list::checkFirstPoint() {
	if(list.size() < 2)
		return;

	float max_distance2 = 80 * 80;

	cwaypoint *first = list[0];
	cwaypoint *second = list[1];
	if(first->loc->dist2xy(second->loc) > max_distance2) {
		list.erase(list.begin());
		delete first;
	}
}
