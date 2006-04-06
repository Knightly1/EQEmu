// zone_list.h: interface for the zone_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ZONE_LIST_H__27ABF001_7A3B_4C6B_9B79_FC791B5A382F__INCLUDED_)
#define AFX_ZONE_LIST_H__27ABF001_7A3B_4C6B_9B79_FC791B5A382F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "czone.h"
#include "../common/linked_list.h"

class zone_list  
{
public:
	zone_list();
	virtual ~zone_list();

	void add( czone* zone );
	czone* get( int pos );
	int getsize() { return size; }

private:
	int size;
	LinkedList<czone*> list;
};

#endif // !defined(AFX_ZONE_LIST_H__27ABF001_7A3B_4C6B_9B79_FC791B5A382F__INCLUDED_)
