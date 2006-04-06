// door_list.cpp: implementation of the door_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "door_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

door_list::door_list()
{
	this->size = 0;
}

void door_list::add( cdoor* door ) {
	list.Append( door );
	size++;
}

cdoor* door_list::get( int pos ) {

    LinkedListIterator<cdoor*> iterator(list);
	iterator.Reset();

	cdoor* door = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			door = iterator.GetData();
			return door;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

door_list::~door_list()
{
	this->list.Clear();
}
