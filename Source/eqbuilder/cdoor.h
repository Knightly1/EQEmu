// cdoor.h: interface for the cdoor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDOOR_H__3D81588B_C866_4830_BC8A_FED16EDF93C4__INCLUDED_)
#define AFX_CDOOR_H__3D81588B_C866_4830_BC8A_FED16EDF93C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cloc.h"

class cdoor  
{
public:
	cdoor();
	virtual ~cdoor();

	int id;
	CString zone;
	int type;
	CString model;
	int teleportid;
	cloc* loc;

};

#endif // !defined(AFX_CDOOR_H__3D81588B_C866_4830_BC8A_FED16EDF93C4__INCLUDED_)
