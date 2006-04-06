// kill_list.cpp: implementation of the kill_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "kill_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

kill_list::kill_list()
{
	this->size = 0;

}

void kill_list::add( ckill* kill ) {
	list.Append( kill );
	size++;
}

ckill* kill_list::get( int pos ) {

    LinkedListIterator<ckill*> iterator(list);
	iterator.Reset();

	ckill* kill = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			kill = iterator.GetData();
			return kill;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

kill_list::~kill_list()
{
	this->list.Clear();
}
