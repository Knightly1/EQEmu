// cwaypoint.h: interface for the cwaypoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CWAYPOINT_H__579D4EB4_61AA_424E_8502_B1938877F09D__INCLUDED_)
#define AFX_CWAYPOINT_H__579D4EB4_61AA_424E_8502_B1938877F09D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cloc.h"

class cwaypoint  
{
public:
	cwaypoint();
	cwaypoint( const cwaypoint* wp );
	virtual ~cwaypoint();
	
	cloc *loc;
	bool pause;
	bool db;
	bool valid;	//if connection from previous to this is valid...
	bool colinear;	//if the connection from previous to this is collinear with the previous link

};

#endif // !defined(AFX_CWAYPOINT_H__579D4EB4_61AA_424E_8502_B1938877F09D__INCLUDED_)
