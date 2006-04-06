// waypoint_list.h: interface for the waypoint_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAYPOINT_LIST_H__3D2E4F62_88C7_4251_AA2E_BE31F6D05F1E__INCLUDED_)
#define AFX_WAYPOINT_LIST_H__3D2E4F62_88C7_4251_AA2E_BE31F6D05F1E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cwaypoint.h"

#include <vector>

using namespace std;

class waypoint_list  
{
public:
	waypoint_list();
	waypoint_list( waypoint_list* wlist );
	virtual ~waypoint_list();

	void add( cwaypoint* wp );
	cwaypoint* get( int pos );
	int getsize() { return list.size(); }
	int nbpauses();
	
	//a special routine added to fix a strange problem
	void checkFirstPoint();

private:
	vector<cwaypoint *> list;
};

#endif // !defined(AFX_WAYPOINT_LIST_H__3D2E4F62_88C7_4251_AA2E_BE31F6D05F1E__INCLUDED_)
