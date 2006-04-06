// teleport_list.h: interface for the teleport_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TELEPORT_LIST_H__3CC0A277_BF9C_4465_8A58_9D616F012C61__INCLUDED_)
#define AFX_TELEPORT_LIST_H__3CC0A277_BF9C_4465_8A58_9D616F012C61__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cteleport.h"
#include "../common/linked_list.h"

class teleport_list  
{
public:
	teleport_list();
	virtual ~teleport_list();

	void add( cteleport* tp );
	cteleport* get( int pos );
	int getsize() { return size; }

private:
	int size;
	LinkedList<cteleport*> list;

};

#endif // !defined(AFX_TELEPORT_LIST_H__3CC0A277_BF9C_4465_8A58_9D616F012C61__INCLUDED_)
