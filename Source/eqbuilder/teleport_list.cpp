// teleport_list.cpp: implementation of the teleport_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "teleport_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

teleport_list::teleport_list()
{
	this->size = 0;

}

void teleport_list::add( cteleport* teleport ) {
	list.Append( teleport );
	size++;
}

cteleport* teleport_list::get( int pos ) {

    LinkedListIterator<cteleport*> iterator(list);
	iterator.Reset();

	cteleport* teleport = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			teleport = iterator.GetData();
			return teleport;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

teleport_list::~teleport_list()
{
	this->list.Clear();
}
