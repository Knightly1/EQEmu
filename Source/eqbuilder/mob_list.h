// mob_list.h: interface for the mob_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MOB_LIST_H__32C0A94E_14A2_4B60_BC80_A5A7A8532FB9__INCLUDED_)
#define AFX_MOB_LIST_H__32C0A94E_14A2_4B60_BC80_A5A7A8532FB9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cmob.h"
#include <vector>

using namespace std;

class mob_list  
{
public:
	mob_list();
	mob_list( mob_list* mobs );
	virtual ~mob_list();

	void add( cmob* mob );
	cmob* get( int pos ) const;
	cmob* getmobbyid( int id, bool can_be_dead = false );
	void remove( cmob* mob );
	int getsize() const { return list.size(); }

private:
	vector<cmob*> list;

};

#endif // !defined(AFX_MOB_LIST_H__32C0A94E_14A2_4B60_BC80_A5A7A8532FB9__INCLUDED_)
