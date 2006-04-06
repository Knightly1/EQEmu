// door_list.h: interface for the door_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DOOR_LIST_H__FCF8C364_91B6_4429_AC61_C441EA67ED50__INCLUDED_)
#define AFX_DOOR_LIST_H__FCF8C364_91B6_4429_AC61_C441EA67ED50__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cdoor.h"
#include "../common/linked_list.h"

class door_list  
{
public:
	door_list();
	virtual ~door_list();

	void add( cdoor* door );
	cdoor* get( int pos );
	int getsize() { return size; }

private:
	int size;
	LinkedList<cdoor*> list;	

};

#endif // !defined(AFX_DOOR_LIST_H__FCF8C364_91B6_4429_AC61_C441EA67ED50__INCLUDED_)
