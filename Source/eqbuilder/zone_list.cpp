// zone_list.cpp: implementation of the zone_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "zone_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

zone_list::zone_list()
{
	this->size = 0;

}

void zone_list::add( czone* zone ) {
	list.Append( zone );
	size++;
}

czone* zone_list::get( int pos ) {

    LinkedListIterator<czone*> iterator(list);
	iterator.Reset();

	czone* zone = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			zone = iterator.GetData();
			return zone;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

zone_list::~zone_list()
{
	this->list.Clear();
}
