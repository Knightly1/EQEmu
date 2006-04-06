// grid_list.cpp: implementation of the grid_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "grid_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

grid_list::grid_list()
{
	this->size = 0;

}

void grid_list::add( cgrid* grid ) {
	list.Append( grid );
	size++;
}

cgrid* grid_list::get( int pos ) {

    LinkedListIterator<cgrid*> iterator(list);
	iterator.Reset();

	cgrid* grid = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			grid = iterator.GetData();
			return grid;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

grid_list::~grid_list()
{
	this->list.Clear();
}
