// loc_list.h: 
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOC_LIST_H__30CEE0CD_A07C_4CEF_B16C_2D68FFE2B620__INCLUDED_)
#define AFX_LOC_LIST_H__30CEE0CD_A07C_4CEF_B16C_2D68FFE2B620__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>

using namespace std;

class cloc;

class loc_list  
{
public:
	loc_list();
	loc_list( loc_list *list );
	virtual ~loc_list();

	void add( cloc* npc );
	cloc* get( int pos );
	int getsize() { return list.size(); }
	
	void getcenter(cloc *center_into);
	void get2Dradius(const cloc *center, float *radius_into);
	
private:
	vector<cloc*> list;

};

#endif // !defined(AFX_NPC_LIST_H__30CEE0CD_A07C_4CEF_B16C_2D68FFE2B620__INCLUDED_)
