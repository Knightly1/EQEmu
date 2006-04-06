// npc_list.h: interface for the npc_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NPC_LIST_H__30CEE0CD_A07C_4CEF_B16C_2D68FFE2B620__INCLUDED_)
#define AFX_NPC_LIST_H__30CEE0CD_A07C_4CEF_B16C_2D68FFE2B620__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cnpc.h"
#include <vector>

using namespace std;

class npc_list  
{
public:
	npc_list();
	virtual ~npc_list();

	void add( cnpc* npc );
	cnpc* get( int pos ) const;
	int getsize() const { return list.size(); }

private:
	vector<cnpc*> list;

};

#endif // !defined(AFX_NPC_LIST_H__30CEE0CD_A07C_4CEF_B16C_2D68FFE2B620__INCLUDED_)
