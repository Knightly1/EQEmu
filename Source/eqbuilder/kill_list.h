// kill_list.h: interface for the kill_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KILL_LIST_H__A9BB59AB_197B_468F_A7A0_FCC719012252__INCLUDED_)
#define AFX_KILL_LIST_H__A9BB59AB_197B_468F_A7A0_FCC719012252__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ckill.h"
#include "../common/linked_list.h"

class kill_list  
{
public:
	kill_list();
	virtual ~kill_list();

	void add( ckill* kill );
	ckill* get( int pos );
	int getsize() { return size; }

private:
	int size;
	LinkedList<ckill*> list;
};

#endif // !defined(AFX_KILL_LIST_H__A9BB59AB_197B_468F_A7A0_FCC719012252__INCLUDED_)
