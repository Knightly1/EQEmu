// spawn_list.h: interface for the spawn_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPAWN_LIST_H__CE2F6174_0480_4CF5_BB9E_10C08F3F3519__INCLUDED_)
#define AFX_SPAWN_LIST_H__CE2F6174_0480_4CF5_BB9E_10C08F3F3519__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cspawn.h"
#include "../common/linked_list.h"

class spawn_list  
{
public:
	spawn_list();
	virtual ~spawn_list();

	void add( cspawn* spawn );
	cspawn* get( int pos );
	int getsize() { return list.size(); }

private:
	vector<cspawn*> list;
};

#endif // !defined(AFX_SPAWN_LIST_H__CE2F6174_0480_4CF5_BB9E_10C08F3F3519__INCLUDED_)
