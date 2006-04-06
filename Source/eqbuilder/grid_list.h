// grid_list.h: interface for the grid_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRID_LIST_H__26D2E8A3_C7C6_4499_8D82_E84546EDB561__INCLUDED_)
#define AFX_GRID_LIST_H__26D2E8A3_C7C6_4499_8D82_E84546EDB561__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cgrid.h"
#include "../common/linked_list.h"

class grid_list  
{
public:
	grid_list();
	virtual ~grid_list();

	void add( cgrid* grid );
	cgrid* get( int pos );
	int getsize() { return size; }

private:
	int size;
	LinkedList<cgrid*> list;
};

#endif // !defined(AFX_GRID_LIST_H__26D2E8A3_C7C6_4499_8D82_E84546EDB561__INCLUDED_)
