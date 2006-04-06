// cgrid.h: interface for the cgrid class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CGRID_H__68C9EAF7_6EF4_4EEA_A471_D10C3F259F8D__INCLUDED_)
#define AFX_CGRID_H__68C9EAF7_6EF4_4EEA_A471_D10C3F259F8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "waypoint_list.h"

class cgrid  
{
public:
	cgrid();
	cgrid( cgrid* grid );
	virtual ~cgrid();

	bool db;
	int id;
	waypoint_list* waypoints;

};

#endif // !defined(AFX_CGRID_H__68C9EAF7_6EF4_4EEA_A471_D10C3F259F8D__INCLUDED_)
